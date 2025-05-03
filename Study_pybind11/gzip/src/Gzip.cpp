#include <iostream>
#include <fstream>
#include <memory>
#include "Gzip.h"
#include "core/src/FileUtils.h"
#include "core/src/ByteBufferIO.h"
#include "core/src/hash.h"
#include "core/src/ThreadPool.h"
#include "core/src/ConcurrentWorker.h"
#include "core/src/string_utils.hpp"
#include "ZlibUtils.h"

#define DEFAUL_HASH_LEN (4 << 20) //4M
#define DEFAUL_HASH_SEED 17

namespace h7_gz {

static inline bool _contains0(const std::vector<String>& vec, CString str){
    for(auto& s : vec){
        if(s == str){
            return true;
        }
    }
    return false;
}

using FUNC_Classify = GzipHelper::FUNC_Classify;
using FUNC_Compressor = GzipHelper::FUNC_Compressor;
using FUNC_DeCompressor = GzipHelper::FUNC_DeCompressor;

String ZipFileItem::readContent()const{
    if(content.empty()){
        return h7::FileUtils::getFileContent(name);
    }
    return content;
}
ZipFileItem ZipFileItem::ofMemoryFile(CString shortName,CString content){
    ZipFileItem zi;
    zi.shortName = shortName;
    zi.content = std::move(content);
    zi.contentLen = content.length();
    int pos = shortName.rfind(".");
    if(pos >= 0){
         zi.ext = shortName.substr(pos + 1);
    }
    return zi;
}

String GroupItem::write(CString buffer) const{
    h7::ByteBufferOut bos(buffer.length() + (1 << 20));
    bos.putInt(children.size());
    for(const ZipFileItem& zi : children){
        bos.putString16(zi.shortName);
    }
    bos.putString16(name);
    size_t hash_len = buffer.size() > DEFAUL_HASH_LEN ? DEFAUL_HASH_LEN : buffer.size();
    uint64 hval = fasthash64(buffer.data(), hash_len, DEFAUL_HASH_SEED);
    bos.putULong(hval);
    bos.putString64(buffer);
    return bos.bufferToString();
}

bool GroupItem::read(String& bufIn, String& bufOut){
    h7::ByteBufferIO bis(&bufIn);
    int childrenCnt = bis.getInt();
    children.resize(childrenCnt);
    for(ZipFileItem& zi : children){
        zi.shortName = bis.getString16();
    }
    this->name = bis.getString16();
    auto hash = bis.getULong();
    bufOut = bis.getString64();

    size_t hash_len = bufOut.size() > DEFAUL_HASH_LEN ? DEFAUL_HASH_LEN : bufOut.size();
    uint64 hval = fasthash64(bufOut.data(), hash_len, DEFAUL_HASH_SEED);
    if(hash != hval){
        return false;
    }
    return true;
}
GroupItem GroupItem::filter(CString ext, bool remove){
    GroupItem ret;
    int size = children.size();
    for(int i = size - 1 ; i >= 0 ; --i){
        auto& zi = children[i];
        if(zi.ext == ext){
            ret.children.push_back(zi);
            if(remove){
                children.erase(children.begin() + i);
            }
        }
    }
    return ret;
}

struct GroupItemTask{
    int index;
    GroupItem* gi;
    String cmpBuffer;

    GroupItemTask(int index, GroupItem* gi,CString buffer):
        index(index), gi(gi), cmpBuffer(buffer){}
};
struct GroupItemState{
    GroupItem gi;
    uint64 bufLen {0};
    bool state {false};
};

struct FileWriter0: public IRandomWriter{

    String path_;
    std::ofstream fos_;

    FileWriter0(CString path):path_(path){
        auto dir = h7::FileUtils::getFileDir(path);
        h7::FileUtils::mkdirs(dir);
    }

    ~FileWriter0(){
        close0();
    }
    bool open() override{
        fos_.open(path_.data(), std::ios::binary);
        return fos_.is_open();
    }
    void seekTo(size_t pos) override{
        fos_.seekp(pos);
    }
    bool write(const String& buf) override{
        size_t len = buf.size();
        if(!write(&len, sizeof(len))){
            return false;
        }
        return write(buf.data(), buf.length());
    }
    bool write(const void* data, size_t len) override{
        fos_.write((char*)data, len);
        return true;
    }
    void close() override{
        close0();
    }
    void close0(){
        if(fos_.is_open()){
            fos_.close();
        }
    }
};

struct MemoryWriter0: public IMemoryWriter{

    h7::ByteBufferOut bos {10 << 20}; //10M

    ~MemoryWriter0(){}

    bool open() override{
        return true;
    }
    void seekTo(size_t /*pos*/) override{
        MED_ASSERT_X(false, "unsupport seekTo for 'ByteBufferOut'");
    }
    bool write(const String& buf) override{
        size_t len = buf.size();
        if(!write(&len, sizeof(len))){
            return false;
        }
        return write(buf.data(), buf.length());
    }
    bool write(const void* data, size_t len) override{
        bos.putData((char*)data, len);
        return true;
    }
    void close() override{
    }
    String getContent() override{
        return bos.bufferToString();
    }
};

std::shared_ptr<IRandomWriter> IRandomWriter::NewFileWriter(CString filePath){
    auto dstDir = h7::FileUtils::getFileDir(filePath);
    h7::FileUtils::mkdirs(dstDir);
    return std::make_shared<FileWriter0>(filePath);
}

struct ZipHeader0{
    String magic {"7NEVAEH"};
    int version {1};
    int groupCount {0};
    std::vector<int> nameLens;
    std::vector<size_t> compressedLens;   

    String str(bool mock)const{
        h7::ByteBufferOut bos(4096);
        bos.putString(magic);
        bos.putInt(version);
        bos.putInt(groupCount);
        if(mock){
            for(int i = 0 ; i < groupCount; ++i){
                bos.putInt(0);
            }
            for(int i = 0 ; i < groupCount; ++i){
                bos.putULong(0);
            }
        }else{
            MED_ASSERT((int)nameLens.size() == groupCount);
            MED_ASSERT((int)compressedLens.size() == groupCount);
            for(auto& len : nameLens){
                bos.putInt(len);
            }
            for(int i = 0 ; i < groupCount; ++i){
                bos.putULong(compressedLens[i]);
            }
        }
        return bos.bufferToString();
    }

    void parse(String& buf){
        nameLens.clear();
        compressedLens.clear();
        h7::ByteBufferIO bio(&buf);
        magic = bio.getString();
        version = bio.getInt();
        groupCount = bio.getInt();
        for(int i = 0 ; i < groupCount; ++i){
            nameLens.push_back(bio.getInt());
        }
        for(int i = 0 ; i < groupCount; ++i){
            compressedLens.push_back(bio.getULong());
        }
    }
};

struct GzipHelper_Ctx0{

    std::vector<String> extFilters;//allowed exts. like "png,jpg,txt...etc"
    std::vector<String> incDirs;
    std::vector<String> excDirs;
    FUNC_Classify func_classify;
    FUNC_Compressor func_compressor;
    FUNC_DeCompressor func_deCompressor;
    int concurrentCnt {1};
    bool debug_ {false};

    bool compressDir(CString dir, CString outFile){
        FileWriter0 fw(outFile);
        return compressDir0(dir, &fw);
    }
    bool compressMemoryFiles(const std::vector<ZipFileItem>& items,
                                         CString outFile){
        FileWriter0 fw(outFile);
        return compressImpl0(items, &fw);
    }
    bool compressFile(CString f, CString outFile){
        FileWriter0 fw(outFile);
        return compressFile0(f, &fw);
    }

    bool compressDirToMemory(CString dir, String& outData){
        MemoryWriter0 mwr;
        if(compressDir0(dir, &mwr)){
            outData = mwr.getContent();
            return true;
        }
        return false;
    }
    bool compressFileToMemory(CString file, String& outData){
        MemoryWriter0 mwr;
        if(compressFile0(file, &mwr)){
            outData = mwr.getContent();
            return true;
        }
        return false;
    }
    bool compressMemoryFilesToMemory(const std::vector<ZipFileItem>& items,
                                     String& outData){
        MemoryWriter0 mwr;
        if(compressImpl0(items, &mwr)){
            outData = mwr.getContent();
            return true;
        }
        return false;
    }
    bool decompressFileToMemory(CString file, std::map<String,String>& out){
        MemoryDecompressManager mdm([](){
            return std::make_shared<MemoryWriter0>();
        });
        if(decompressFile0(file, &mdm)){
            out = mdm.getItemMap();
            return true;
        }
        return false;
    }
    bool decompressFile(CString file, CString outDir){
        FileDecompressManager fdm(outDir);
        return decompressFile0(file, &fdm);
    }
    bool decompressFile0(CString file, IDecompressManager* decM){
        const auto fileTotalLen = h7::FileUtils::getFileSize(file);
        if(fileTotalLen == 0){
            fprintf(stderr, "file is empty: %s\n", file.data());
            return false;
        }
        std::ifstream fis;
        fis.open(file, std::ios::binary);
        if(!fis.is_open()){
            return false;
        }
        size_t contentPos = 0;
        size_t contentLen = 0;
        ZipHeader0 header;
        //read head
        {
            size_t headerSize = 0;
            fis.read((char*)&headerSize, sizeof(size_t));
            if(fis.fail() || headerSize == 0){
                return false;
            }
            contentPos = headerSize + sizeof (size_t);
            contentLen = fileTotalLen - contentPos * 2;
            //
            size_t headerActPos = contentPos + contentLen + sizeof(size_t);
            fis.seekg(headerActPos, std::ios::beg);
            if(fis.fail()){
                return false;
            }
            //
            String headBuf;
            headBuf.resize(headerSize);
            fis.read((char*)headBuf.data(), headerSize);
            if(fis.fail()){
                return false;
            }
            header.parse(headBuf);
            if((int)header.compressedLens.size() != header.groupCount){
                return false;
            }
            if((int)header.nameLens.size() != header.groupCount){
                return false;
            }
            fis.seekg(contentPos, std::ios::beg);
            if(fis.fail()){
                return false;
            }
        }
        //read body.
        std::vector<std::shared_ptr<GroupItemState>> gitems;
        {
            h7::ThreadPool pool(concurrentCnt);
            for(int ni = 0; ni < (int)header.compressedLens.size(); ++ ni){
                size_t blockSize = 0;
                fis.read((char*)&blockSize, sizeof(size_t));
                if(fis.fail()){
                    break;
                }
                auto bufPtr = std::shared_ptr<String>(new String());
                bufPtr->resize(blockSize);
                fis.read((char*)bufPtr->data(), blockSize);
                if(fis.fail()){
                    return false;
                }
                auto gs = std::make_shared<GroupItemState>();
                gitems.push_back(gs);
                pool.enqueue([this, decM, bufPtr, gs](){
                    std::vector<String> datas;
                    {
                        String bufOut;
                        gs->gi.read(*bufPtr, bufOut);
                        if(!func_deCompressor(bufOut, datas)){
                            gs->state = false;
                            return;
                        }
                        gs->bufLen = bufOut.size();
                        if(gs->gi.children.size() != datas.size()){
                            gs->state = false;
                            return;
                        }
                        gs->state = true;
                    }
                    for(int i = 0 ; i < (int)datas.size() ; ++i){
                        auto& shortName = gs->gi.children[i].shortName;
                        auto writer0 = decM->getWriter(shortName);
                        if(writer0->open()){
                            auto& dat = datas[i];
                            writer0->write(dat.data(), dat.size());
                            writer0->close();
                        }else{
                            fprintf(stderr, "write file failed. %s\n", shortName.data());
                        }
                    }
                    if(debug_){
                        for(int i = 0 ; i < (int)datas.size() ; ++i){
                             auto& zi = gs->gi.children[i];
                             auto& cs = datas[i];
                             auto hash = fasthash64(cs.data(), cs.length(), DEFAUL_HASH_SEED);
                             auto hashStr = std::to_string(hash);
                             printf("[ DeCompress ] %s: hash = %s\n", zi.shortName.data(), hashStr.data());
                        }
                    }
                });
            }
        }
        //verify
        if(header.groupCount != (int)gitems.size()){
            return false;
        }
        for(int i = 0 ; i < header.groupCount ; ++i){
            if(!gitems[i]->state){
                return false;
            }
            if((int)gitems[i]->gi.name.length() != header.nameLens[i]){
                return false;
            }
            if(gitems[i]->bufLen != header.compressedLens[i]){
                return false;
            }
        }
        return true;
    }

private:
    bool compressDir0(CString dir, IRandomWriter* rw){
        std::vector<String> vec;
        std::vector<String> exts;
        {
            auto files = h7::FileUtils::getFiles(dir, true, "");
            if(files.empty()){
                fprintf(stderr, "compressDir >> dir is empty. %s\n", dir.data());
                return false;
            }
            vec.reserve(files.size());
            exts.reserve(files.size());
            //filter
            if(!extFilters.empty()){
                for(auto& f: files){
                    String ext;
                    int pos = f.rfind(".");
                    if(pos >= 0){
                        ext = f.substr(pos + 1);
                    }
                    if(_contains0(extFilters, ext)){
                        vec.push_back(f);
                        exts.push_back(ext);
                    }
                }
            }else if(!incDirs.empty()){
                for(auto& f: files){
                    String ext;
                    int pos = f.rfind(".");
                    if(pos >= 0){
                        ext = f.substr(pos + 1);
                    }
                    for(auto& incDir: incDirs){
                        if(h7::utils::startsWith(f, incDir)){
                            vec.push_back(f);
                            exts.push_back(ext);
                            break;
                        }
                    }
                }
            }else if(!excDirs.empty()){
                for(auto& f: files){
                    String ext;
                    int pos = f.rfind(".");
                    if(pos >= 0){
                        ext = f.substr(pos + 1);
                    }
                    bool inc = false;
                    for(auto& incDir: excDirs){
                        if(h7::utils::startsWith(f, incDir)){
                            inc = true;
                            break;
                        }
                    }
                    if(!inc){
                        vec.push_back(f);
                        exts.push_back(ext);
                    }
                }
            }
            else{
                vec = std::move(files);
                for(auto& f : vec){
                    String ext;
                    int pos = f.rfind(".");
                    if(pos >= 0){
                        ext = f.substr(pos + 1);
                    }
                    exts.push_back(ext);
                }
            }
        }
        if(vec.empty()){
            fprintf(stderr, "no files to compress.\n");
            return false;
        }
        return compressImpl1(dir, vec, exts, rw);
    }
    bool compressFile0(CString f, IRandomWriter* rw){
        std::vector<String> vec;
        std::vector<String> exts;
        String ext;
        int pos = f.rfind(".");
        if(pos >= 0){
            ext = f.substr(pos + 1);
        }
        exts.push_back(ext);
        vec.push_back(f);
        //
        return compressImpl1(h7::FileUtils::getFileDir(f), vec, exts, rw);
    }
    bool compressImpl1(CString dir, std::vector<String>& vec,
                       std::vector<String>& exts, IRandomWriter* writer){
        std::vector<ZipFileItem> items;
        for(int i = 0 ; i < (int)vec.size() ; ++i){
            ZipFileItem fi;
            fi.ext = exts[i];
            fi.contentLen = h7::FileUtils::getFileSize(vec[i]);
            fi.name = vec[i];
            fi.shortName = fi.name.substr(dir.length() + 1);
            items.push_back(std::move(fi));
        }
        return compressImpl0(items, writer);
    }

    bool compressImpl0(std::vector<ZipFileItem> items, IRandomWriter* writer){
        std::vector<GroupItem> gitems;
        {
            //category
            if(items.size() > 1 && func_classify){
                func_classify(items, gitems);
            }else{
                GroupItem gi;
                gi.name = "Default";
                gi.children = std::move(items);
                gitems.push_back(std::move(gi));
            }
            if(debug_){
                for(auto& gis : gitems){
                    printf("--- group.name : '%s'\n", gis.name.data());
                    for(auto& zi :gis.children){
                        auto cs = zi.readContent();
                        auto hash = fasthash64(cs.data(), cs.length(), DEFAUL_HASH_SEED);
                        auto hashStr = std::to_string(hash);
                        printf("[ Compress ] %s: hash = %s\n", zi.shortName.data(), hashStr.data());
                    }
                }
            }
        }
        //compress
        ZipHeader0 header;
        header.groupCount = gitems.size();
        if(!writer->open()){
            return false;
        }
        if(!writer->write(header.str(true))){
            return false;
        }
        if(concurrentCnt > 1){
            using SpGITask = std::shared_ptr<GroupItemTask>;
            auto writer0 = writer;
            std::vector<int> writeRets(gitems.size(), 1);
            h7::ConcurrentWorker<SpGITask> worker(gitems.size(),
                                                [this, writer0, &header, &writeRets](SpGITask& st){
                 writeRets[st->index] = doWrite(writer0, header, st->gi, st->cmpBuffer);
            });
            worker.start();
            //multi threads compress.
            //write to a temp file. then merge?. NO IO may be too lot.
            int ret = h7::ThreadPool::batchRawRun(concurrentCnt, 0, gitems.size(),
                                                      [this, &gitems, &worker](int i){
                    auto& children = gitems[i].children;
                    //auto& key = gitems[i].name;
                    String buffer;
                    if(!func_compressor(children, &buffer)){
                        return false;
                    }
                    if(buffer.empty()){
                        return false;
                    }
                    worker.addTask(std::make_shared<GroupItemTask>(i, &gitems[i], std::move(buffer)));
                    return true;
                });
            //stop write worker
            worker.stop();
            if(ret == 0){
                return false;
            }
            MED_ASSERT(writeRets.size() == gitems.size());
            for(auto& writeR: writeRets){
                if(!writeR){
                    return false;
                }
            }
        }else{
            for(auto it = gitems.begin(); it != gitems.end(); ++it){
                auto& children = it->children;
                //auto& key = it->name;
                //
                String buffer;
                func_compressor(children, &buffer);
                if(buffer.empty()){
                    return false;
                }
                auto& gitem = *it;
                if(!doWrite(writer, header, &gitem, buffer)){
                    return false;
                }
            }
        }
        {
            auto hstr = header.str(false);
            //printf("write header: %s\n", hstr.data());
            if(!writer->write(hstr)){
                return false;
            }
        }
        writer->close();
        return true;
    }
    bool doWrite(IRandomWriter* writer, ZipHeader0& header,
                 GroupItem* gi, CString buffer){
        String data = gi->write(buffer);
        {
            std::unique_lock<std::mutex> lck(_mtx_write);
            if(!writer->write(data)){
                return false;
            }
            header.nameLens.push_back(gi->name.size());
            header.compressedLens.push_back(buffer.size());
        }
        return true;
    }

private:
    std::mutex _mtx_write;
};
//---------------------------

GzipHelper::GzipHelper(){
    m_ptr = new GzipHelper_Ctx0();
    setCompressor([](const std::vector<ZipFileItem>& items, String* out){
        unsigned long long mayTotalSize = sizeof(int);
        for(auto& zi : items){
            mayTotalSize += zi.contentLen;
            mayTotalSize += sizeof(unsigned long long);
        }
        h7::ByteBufferOut bos(mayTotalSize);
        bos.putInt(items.size());
        for(auto& zi : items){
            auto cs = zi.readContent();
            bos.putString64(cs);
        }
        auto buffer = bos.bufferToString();
        return ZlibUtils::compress(buffer, *out);
    });
    setDeCompressor([](String& _str,std::vector<String>& vecOut){
        String str;
        if(!ZlibUtils::decompress(_str, str)){
            return false;
        }
        h7::ByteBufferIO bis(&str);
        int size = bis.getInt();
        vecOut.resize(size);
        for(int i = 0 ; i < size ; ++i){
            vecOut[i] = bis.getString64();
        }
        return true;
    });
}
GzipHelper::~GzipHelper(){
    if(m_ptr){
        delete m_ptr;
        m_ptr = nullptr;
    }
}
void GzipHelper::setUseSimpleEncDec(){
    setCompressor([](const std::vector<ZipFileItem>& items, String* out){
        unsigned long long mayTotalSize = sizeof(int);
        for(auto& zi : items){
            mayTotalSize += zi.contentLen;
            mayTotalSize += sizeof(unsigned long long);
        }
        h7::ByteBufferOut bos(mayTotalSize);
        bos.putInt(items.size());
        for(auto& zi : items){
            auto cs = zi.readContent();
            bos.putString64(cs);
        }
        *out = bos.bufferToString();
        std::reverse(out->begin(), out->end());
        return true;
    });
    setDeCompressor([](String& str,std::vector<String>& vecOut){
        std::reverse(str.begin(), str.end());
        h7::ByteBufferIO bis(&str);
        int size = bis.getInt();
        vecOut.resize(size);
        for(int i = 0 ; i < size ; ++i){
            vecOut[i] = bis.getString64();
        }
        return true;
    });
}
void GzipHelper::setClassifier(FUNC_Classify func){
    m_ptr->func_classify = func;
}
void GzipHelper::setCompressor(FUNC_Compressor func){
    m_ptr->func_compressor = func;
}
void GzipHelper::setDeCompressor(FUNC_DeCompressor func){
    m_ptr->func_deCompressor = func;
}
void GzipHelper::setConcurrentThreadCount(int count){
    m_ptr->concurrentCnt = count;
}
void GzipHelper::setAttentionFileExtensions(const std::vector<String>& exts){
    m_ptr->extFilters = exts;
}
void GzipHelper::setExcludeDirs(const std::vector<String>& v){
    m_ptr->excDirs = v;
}
void GzipHelper::setDebug(bool debug){
    m_ptr->debug_ = debug;
}
bool GzipHelper::compressDir(CString dir, CString outFile){
    return m_ptr->compressDir(dir, outFile);
}
bool GzipHelper::compressFile(CString file, CString outFile){
    return m_ptr->compressFile(file, outFile);
}
bool GzipHelper::decompressFile(CString file, CString outDir){
    return m_ptr->decompressFile(file, outDir);
}
bool GzipHelper::decompressFileToMemory(CString file, std::map<String,String>& out){
    return m_ptr->decompressFileToMemory(file, out);
}
bool GzipHelper::compressMemoryFiles(const std::vector<ZipFileItem>& items,
                                     CString outFile){
    return m_ptr->compressMemoryFiles(items, outFile);
}
bool GzipHelper::compressDirToMemory(CString dir, String& outData){
    return m_ptr->compressDirToMemory(dir, outData);
}
bool GzipHelper::compressFileToMemory(CString file, String& outData){
    return m_ptr->compressFileToMemory(file, outData);
}
bool GzipHelper::compressMemoryFilesToMemory(const std::vector<ZipFileItem>& items,
                                 String& outData){
    return m_ptr->compressMemoryFilesToMemory(items, outData);
}
//
}
