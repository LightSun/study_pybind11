#include "CacheManager.h"
#include <memory.h>
#include "snappy/snappy.h"
#include "common.h"
#include "msvc_compat.h"

using namespace h7;

static inline void readBigFile(CString rfile, std::vector<char>& _buffer){
    FILE* stream_in = fopen64(rfile.data(), "rb");
    if(stream_in == NULL){
        return;
    }
    //MED_ASSERT_X(stream_in != NULL, "read file failed: " + rfile);
    fseeko64(stream_in, 0, SEEK_END);
    uint64 _size = ftello64(stream_in);
    _buffer.resize(_size);
    fseeko64(stream_in, 0, SEEK_SET);
    fread(_buffer.data(), 1, _buffer.size(), stream_in);
    fclose(stream_in);
}

void CacheManager::addFileItem(const std::string& name, const std::string& filePath){
    std::vector<char> buf;
    readBigFile(filePath, buf);
    addItem(name, buf.data(), buf.size());
}
void CacheManager::addFileItemCompressed(const std::string& name, const std::string& filePath){
    std::vector<char> buf;
    readBigFile(filePath, buf);
    addItemCompressed(name, buf.data(), buf.size());
}
void CacheManager::addItem(const std::string& name, const char* data, uint64 len,
                           uint32 flags){
    Item item;
    memcpy(item.name, name.data(), name.length() + 1);
    item.raw_size = len;
    item.flags = flags;
    //
    auto usedSize = getLastFragUsedSize();
    if(usedSize == 0){
        auto frag_id = m_data.size();
        auto left = len;
        uint64 ptr_offset = 0;
        while (left > 0) {
            auto _size = HMIN(left, m_maxFragSize);
            auto& _block = newBlock(_size);
            memcpy(_block.data(), data + ptr_offset, _size);
            ptr_offset += _size;
            left -= _size;
            //frag id
            item.frag_ids.push_back(frag_id);
            frag_id ++;
        }
        item.frag_offset = 0;
    }else{
        auto last_free_size = m_maxFragSize - usedSize;
        //data is small.
        if(len <= last_free_size){
            auto& block = getLastBlock(usedSize + len);
            memcpy(block.data() + usedSize, data, len);
            //
            item.frag_ids.push_back(getLastBlockId());
            item.frag_offset = usedSize;
        }else{
            //add some data to old frag. and left data to new frag
            auto& block = getLastBlock(m_maxFragSize);
            memcpy(block.data() + usedSize, data, last_free_size);
            //first frag id
            auto frag_id = getLastBlockId();
            item.frag_ids.push_back(frag_id);
            item.frag_offset = usedSize;
            frag_id++;
            //
            auto left = len - last_free_size;
            uint64 ptr_offset = last_free_size;
            while (left > 0) {
                auto _size = HMIN(left, m_maxFragSize);
                auto& _block = newBlock(_size);
                memcpy(_block.data(), data + ptr_offset, _size);
                ptr_offset += _size;
                left -= _size;
                //frag id
                item.frag_ids.push_back(frag_id);
                frag_id ++;
            }
        }
    }
    m_items.push_back(std::move(item));
}
void CacheManager::addItemCompressed(const std::string& name, const char* data,
                                     uint64 len){
    std::string str;
    snappy::Compress((const char*)data, len, &str);
    addItem(name, str.data(), str.length(), kFlag_COMPRESSED);
}

void CacheManager::getItemDataUnCompressed(const std::string& name, std::string& out){
    auto _idx = getItemByName(name);
    if(_idx < 0){
        return;
    }
    if( (m_items[_idx].flags & kFlag_COMPRESSED) != 0){
        std::string _out;
        getItemData0(_idx, _out);
        snappy::Uncompress(_out.data(), _out.length(), &out);
    }else{
        getItemData0(_idx, out);
    }
}

void CacheManager::getItemAt(unsigned int _idx, std::string& o_name, std::string& o_data){
    MED_ASSERT(_idx < m_items.size());
    o_name = m_items[_idx].name;
    if( (m_items[_idx].flags & kFlag_COMPRESSED) != 0){
        std::string _out;
        getItemData0(_idx, _out);
        snappy::Uncompress(_out.data(), _out.length(), &o_data);
    }else{
        getItemData0(_idx, o_data);
    }
}

void CacheManager::getItemData(CString name, std::string& out){
    auto _idx = getItemByName(name);
    if(_idx < 0){
        return;
    }
    getItemData0(_idx, out);
}

void CacheManager::removeItem(CString name){
    auto _idx = getItemByName(name);
    if(_idx < 0){
        return;
    }
    Item* _item = &m_items[_idx];
    if(_item->frag_offset == 0){
        auto firstId = _item->frag_ids[0];
        int rm_count = _item->raw_size / m_maxFragSize;
        //check last block need remove or not.
        auto lastId = _item->frag_ids[_item->frag_ids.size()-1];
        if(lastId != firstId && m_data[lastId].size() == (_item->raw_size % m_maxFragSize)){
            rm_count ++;
        }
        //
        for(int i = rm_count -1 ; i >= 0 ; --i){
            m_data.erase(m_data.begin() + firstId + i);
        }
        m_items.erase(m_items.begin() + _idx);
        //sync frag ids after items id
        for(uint32 i = 0 ; i < m_items.size() ; i ++){
            for(auto& id : m_items[i].frag_ids){
                 if(id >= firstId){
                     id -= rm_count;
                 }
            }
        }
        return;
    }
    if(_item->frag_ids.size() > 1){
        //first block don't need remove.
        uint32 firstId = _item->frag_ids[1];
        uint32 lastId = _item->frag_ids[_item->frag_ids.size() - 1];
        auto left_size = _item->raw_size - _item->frag_offset;
        int rm_count = left_size / m_maxFragSize;
        //check last block.
        if(lastId != firstId && m_data[lastId].size() == (left_size % m_maxFragSize)){
            rm_count ++;
        }
        //remove blocks
        for(int i = rm_count -1 ; i >= 0 ; --i){
            m_data.erase(m_data.begin() + firstId + i);
        }
        if(_idx == (int)m_items.size() - 1){
            //already the last item. no id need dec.
            m_items.erase(m_items.begin() + _idx);
        }else{
            m_items.erase(m_items.begin() + _idx);
            //sync frag ids after items id
            for(uint32 i = 0 ; i < m_items.size() ; i ++){
                for(auto& id : m_items[i].frag_ids){
                     if(id >= firstId){
                         id -= rm_count;
                     }
                }
            }
        }
    }else{
        //only on one block and have other item. no need remove block
        m_items.erase(m_items.begin() + _idx);
    }
}

uint32 CacheManager::getItemCount(){
    return m_items.size();
}
std::vector<String> CacheManager::getItemNames(){
    std::vector<String> ret;
    ret.reserve(m_items.size());
    for(auto& item : m_items){
        ret.push_back(item.name);
    }
    return ret;
}

uint64 CacheManager::computeRecordSize(){
    uint64 _size = 0;
    _size += sizeof(uint32) * 2; //block_count + item_count
    _size += sizeof(bool); //compress or not
    uint32 item_count = m_items.size();
    for(uint32 i = 0 ; i < item_count ; i ++){
        _size += __CACHE_NAME_SIZE;
        _size += (sizeof(uint32) + sizeof(uint64) * 2);  //flags + raw_size + frag_offset
        _size += sizeof(int); //id count
        _size += m_items[i].frag_ids.size() * sizeof(uint32); //all ids
    }
    return _size;
}
void CacheManager::writeRecordFile(CString _file, bool compressed){
    //write record
    uint32 item_count = m_items.size();
    uint32 block_count = m_data.size();
    std::vector<char> _buffer;
    _buffer.resize(computeRecordSize());
    //write count info
    uint32 offset = 0;
    memcpy(_buffer.data(), &item_count, sizeof(uint32));
    offset += sizeof(uint32);
    memcpy(_buffer.data() + offset, &block_count, sizeof(uint32));
    offset += sizeof(uint32);
    memcpy(_buffer.data() + offset, &compressed, sizeof(bool));
    offset += sizeof(bool);
    //
    for(uint32 i = 0 ; i < item_count ; i ++){
         //name
         memcpy(_buffer.data() + offset, m_items[i].name, __CACHE_NAME_SIZE);
         offset += __CACHE_NAME_SIZE;
         //flags
         memcpy(_buffer.data() + offset, &m_items[i].flags, sizeof(uint32));
         offset += sizeof(uint32);
         //raw_size
         memcpy(_buffer.data() + offset, &m_items[i].raw_size, sizeof(uint64));
         offset += sizeof(uint64);
         //frag_offset
         memcpy(_buffer.data() + offset, &m_items[i].frag_offset, sizeof(uint64));
         offset += sizeof(uint64);
         //frag ids
         int id_count = m_items[i].frag_ids.size();
         memcpy(_buffer.data() + offset, &id_count, sizeof(int));
         offset += sizeof(int);

         memcpy(_buffer.data() + offset, m_items[i].frag_ids.data(),
                id_count * sizeof(uint32));
         offset += id_count * sizeof(uint32);
    }
    //write
    FILE* stream_out = fopen64(_file.data(), "wb");
    fwrite(_buffer.data(), 1, _buffer.size(), stream_out);
    fflush(stream_out);
    fclose(stream_out);
}

void CacheManager::saveTo(CString dir,CString recordName, CString dataName){
    if(m_items.empty()){
        return;
    }
    std::string outDir_pre = dir.empty() ? "" : dir + "/";
    for(uint32 i = 0 ; i < (uint32)m_data.size() ; i ++){
        MED_ASSERT(!m_data[i].empty());
        std::string out_file = outDir_pre + dataName + std::to_string(i) + ".dt";
        FILE* stream_out = fopen64(out_file.data(), "wb");
        fwrite(m_data[i].data(), 1, m_data[i].size(), stream_out);
        fflush(stream_out);
        fclose(stream_out);
    }
    //write record
    std::string out_file = outDir_pre + recordName + ".dr";
    writeRecordFile(out_file, false);
}

void CacheManager::compressTo(CString dir,CString recordName, CString dataName){
    if(m_items.empty()){
        return;
    }
    std::string outDir_pre = dir.empty() ? "" : dir + "/";
    uint32 block_count = m_data.size();
    for(uint32 i = 0 ; i < block_count ; i ++){
        MED_ASSERT(!m_data[i].empty());
        std::string out_file = outDir_pre + dataName + std::to_string(i) + ".dt";
        std::string real_data;
        auto _csize = snappy::Compress(m_data[i].data(), m_data[i].size(), &real_data);
        MED_ASSERT(_csize == real_data.length());
        FILE* stream_out = fopen64(out_file.data(), "wb");
        fwrite(real_data.data(), 1, _csize, stream_out);
        fflush(stream_out);
        fclose(stream_out);
    }
    //write record
    std::string out_file = outDir_pre + recordName + ".dr";
    writeRecordFile(out_file, true);
}

bool CacheManager::load(CString dir,CString recordName, CString dataName){
     std::string outDir_pre = dir.empty() ? "" : dir + "/";
     std::string rfile = outDir_pre + recordName + ".dr";
     std::vector<char> _buffer;
     readBigFile(rfile, _buffer);
     if(_buffer.empty()){
         return false;
     }
     //
     uint64 offset = 0;
     uint32 block_count;
     uint32 item_count;
     bool compressed;
     memcpy(&item_count, _buffer.data(), sizeof(uint32));
     offset += sizeof(uint32);
     memcpy(&block_count, _buffer.data() + offset, sizeof(uint32));
     offset += sizeof(uint32);
     memcpy(&compressed, _buffer.data() + offset, sizeof(bool));
     offset += sizeof(bool);
     //no flag
     m_items.resize(item_count);
     int id_count;
     for(uint32 i = 0 ; i < item_count ; i ++){
         //name, flags, raw_size, frag_offset, ids
        memcpy(m_items[i].name, _buffer.data() + offset, __CACHE_NAME_SIZE);
        offset += __CACHE_NAME_SIZE;
        memcpy(&m_items[i].flags, _buffer.data() + offset, sizeof(uint32));
        offset += sizeof(uint32);
        memcpy(&m_items[i].raw_size, _buffer.data() + offset, sizeof(uint64));
        offset += sizeof(uint64);
        memcpy(&m_items[i].frag_offset, _buffer.data() + offset, sizeof(uint64));
        offset += sizeof(uint64);
        //
        memcpy(&id_count, _buffer.data() + offset, sizeof(int));
        offset += sizeof(int);
        m_items[i].frag_ids.resize(id_count);
        memcpy(m_items[i].frag_ids.data(), _buffer.data() + offset,
               id_count * sizeof(uint32));
        offset += id_count * sizeof(uint32);
     }
     //data file
     m_data.resize(block_count);
     for(uint32 i = 0 ; i < block_count ; i ++){
         std::string out_file = outDir_pre + dataName + std::to_string(i) + ".dt";
         if(compressed){
             std::vector<char> vec;
             readBigFile(out_file, vec);

             std::string str;
             MED_ASSERT(snappy::Uncompress(vec.data(), vec.size(), &str));
             m_data[i].resize(str.length());
             memcpy(m_data[i].data(), str.data(), str.length());
         }else{
             readBigFile(out_file, m_data[i]);
         }
     }
     return true;
}
//-------------------------
void CacheManager::getItemData0(int _idx, std::string& out){
    Item* _item = &m_items[_idx];
    out.resize(_item->raw_size);
    //
    uint64 ptr_offset = 0;
    uint64 left_size = _item->raw_size;
    bool first = true;
    for(auto id: _item->frag_ids){
        if(first){
            first = false;
            auto _size = HMIN(left_size, m_maxFragSize - _item->frag_offset);
            memcpy((void*)(out.data() + ptr_offset), m_data[id].data() + _item->frag_offset, _size);
            left_size -= _size;
            ptr_offset += _size;
        }else{
            auto _size = HMIN(left_size, m_maxFragSize);
            memcpy((void*)(out.data() + ptr_offset), m_data[id].data(), _size);
            left_size -= _size;
            ptr_offset += _size;
        }
    }
}
