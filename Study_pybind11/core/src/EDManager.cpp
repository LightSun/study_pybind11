#ifdef _MSC_VER
#pragma warning(disable: 4786)
#include <random>
#endif

#include <memory>
#include <algorithm>
#include "EDManager.h"
#include "FileUtils.h"
#include "string_utils.hpp"
#include "PerformanceHelper.h"
#include "hash.h"
#include "CacheManager.h"
#include "common.h"
#include "helpers.h"
#include "CmdBuilder2.h"
#include "collections.h"

#ifdef _MSC_VER
namespace std {

template <class _RanIt>
static void random_shuffle(_RanIt _First, _RanIt _Last) {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(_First, _Last, g);
}
}
#endif

#ifndef DISABLE_ONNX2TRT
#include "Med_ai_acc/med_infer_ctx.h"
#endif

#ifdef WITH_ONNX_PARSER
#include "OnnxParser.h"
#endif

#define __INTERNAL_PREFIX "__$("
#define __SALT_KEY __INTERNAL_PREFIX"SALT)"

using namespace med_qa;
namespace _h7 {
struct MergeItem{
    String name;
    int idxMain {-1};
    int idxSub {-1};
};
static inline int indexOf0(std::vector<String>& l, CString s){
    for(int i = 0 ; i < (int)l.size() ; ++i){
        if(l[i] == s) return i;
    }
    return -1;
}
}
template<typename T>
static inline String formatDimsImpl(const std::vector<T>& vec,
                                    bool necToOne,
                                    CString sep = "x"){
    String str;
    for(size_t i = 0 ; i < vec.size() ; ++i){
        int val = vec[i];
        if(val == -1 && necToOne){
            val = 1;
        }
        str.append(std::to_string(val));
        if(i != vec.size() - 1){
            str.append(sep);
        }
    }
    return str;
}

//EDManager::EDManager(CString encFileDirs, CString encFileSuffixes, CString encOutDesc):
//    m_encFileDirs(encFileDirs),m_encFileSuffixes(encFileSuffixes),
//    m_encOutDesc(encOutDesc){
//    if(m_encOutDesc.find(",") == String::npos){
//        m_encOutDesc = m_encOutDesc + ",record,data";
//    }
//}

EDManager::~EDManager(){
    if(m_cacheM){
        delete m_cacheM;
        m_cacheM = nullptr;
    }
}
void EDManager::compress(bool verify){
    MED_ASSERT(!m_encFileDirs.empty());
    MED_ASSERT(!m_encFileSuffixes.empty());
    MED_ASSERT(!m_encOutDesc.empty());
    //
    h7::PerformanceHelper ph;
    ph.begin();
    //encFileDirs.
    using namespace h7;
    List<String> files;
    List<String> keys;
    {
    auto dirs = h7::utils::split(",", m_encFileDirs);
    auto suffixes = h7::utils::split(",", m_encFileSuffixes);
    for(auto& dir : dirs){
        List<String> l;
        for(auto& suf : suffixes){
            auto list = FileUtils::getFiles(dir, true, suf);
            //check ignore
            for(auto& str : list){
                //prefer inc path
                if(m_incPaths.empty()){
                    if(!shouldIgnore(str)){
                        l.push_back(str);
                    }
                }else{
                    if(shouldInclude(str)){
                        l.push_back(str);
                    }
                }
            }
        }
        files.insert(files.end(), l.begin(), l.end());
        //
        for(auto& s : l){
            keys.push_back(s.substr(dir.length()));
        }
    }
    }
    compress(files, keys, verify);
}
void EDManager::compress(CList<String> files, CList<String> keys,bool verify){
    using namespace h7;
    auto desc = h7::utils::split(",", m_encOutDesc);
    MED_ASSERT(desc.size() == 3);
    //
    m_keys = keys;
    h7::PerformanceHelper ph;
    ph.begin();
    //dir,recordName,dataName
    //
    {
        CacheManager cm(2 << 30); //max 2G
        int size = files.size();
        for(int i = 0 ; i < size ; ++i){
            cm.addFileItem(m_keys[i], files[i]);
        }
        //key id item.
        cm.compressTo(desc[0], desc[1], desc[2]);
        ph.print("compress");
    }
#ifdef WITH_ONNX_PARSER
    {
        m_inputNames.clear();
        m_inputShapes.clear();

        int size = files.size();
        h7_onnx::OnnxParser parser;
        bool ok = true;
        for(int i = 0 ; i < size ; ++i){
            if(!parser.parseFromFile(files[i])){
                ok = false;
                break;
            }
            int c = parser.getInputTensorCount();
            if(c <= 0){
                ok = false;
                break;
            }
            auto vec = parser.getInputDims(0);
            m_inputShapes.push_back(formatDimsImpl(vec, false));
            m_inputNames.push_back(parser.getInputName(0));
        }
        if(!ok){
            m_inputNames.clear();
            m_inputShapes.clear();
        }
    }
#endif
    //
    if(verify){
        ph.begin();
        CacheManager cm(2 << 30);
        cm.load(desc[0], desc[1], desc[2]);
        int size = files.size();
        for(int i = 0 ; i < size ; ++i){
            String rawContent = FileUtils::getFileContent(files[i]);
            String out;
            cm.getItemData(m_keys[i], out);
            //fasthash64();
            if(rawContent != out){
                fprintf(stderr, " verify >> enc-dec failed, key = '%s'\n", m_keys[i].data());
            }
        }
        ph.print("verify");
    }
}
void EDManager::loadDir(CString dir){
    String desc = dir + ",record,data";
    load(desc);
}
void EDManager::load(CString _encOutDesc){
    String encOutDesc = _encOutDesc;
    if(encOutDesc.find(",") == String::npos){
        encOutDesc = encOutDesc + ",record,data";
    }
    h7::PerformanceHelper ph;
    ph.begin();
    auto desc = h7::utils::split(",", encOutDesc);
    MED_ASSERT_X(desc.size() == 3, encOutDesc);
    //
    MED_ASSERT_X(h7::FileUtils::isFileExists(desc[0]), desc[0]);
    //
    using namespace h7;
    if(m_cacheM){
        delete m_cacheM;
        m_cacheM = nullptr;
    }
    m_cacheM = new CacheManager(2 << 30);
    m_cacheM->load(desc[0], desc[1], desc[2]);
    //check salt
    String salt;
    m_cacheM->getItemData(__SALT_KEY, salt);
    if(!salt.empty()){
        auto newCM = std::unique_ptr<CacheManager>(new h7::CacheManager(2 << 30));
        int c = m_cacheM->getItemCount();
        for(int i = 0 ; i < c ; ++i){
        //for(int i :MFOREACH(c)){
            String name;
            String data;
            m_cacheM->getItemAt(i, name, data);
            if(name != __SALT_KEY){
                if(name.find(__INTERNAL_PREFIX) != String::npos){
                    newCM->addItem(name, data);
                    continue;
                }
                std::reverse(data.begin(), data.end());
                int offset = 0;
                if(data.data()[0] == '1'){
                    size_t saltLen = 0;
                    memcpy(&saltLen, data.data() + 1, sizeof(size_t));
                    offset = 1 + sizeof(size_t) + saltLen;
                }else{
                    offset = 1;
                }
                newCM->addItem(name, String(data.data() + offset, data.length() - offset));
            }
        }
        delete m_cacheM;
        m_cacheM = newCM.release();
    }
    ph.print("loadED");
}
String EDManager::getItem(CString key){
    if(!m_cacheM){
        return "";
    }
    String out;
    m_cacheM->getItemData(key, out);
    return out;
}
void EDManager::addItem(CString key, CString data){
    if(!m_cacheM){
        m_cacheM = new h7::CacheManager(2 << 30);
    }
    m_cacheM->removeItem(key);
    m_cacheM->addItem(key, data);
}
void EDManager::removeItem(CString key){
    if(m_cacheM){
        m_cacheM->removeItem(key);
    }
}
void EDManager::removeItemIfExclude(CList<String> keys){
    if(!m_cacheM){
        return;
    }
    int count = m_cacheM->getItemCount();
    for(int i = count - 1 ; i >= 0 ; --i){
        String key, data;
        m_cacheM->getItemAt(i, key, data);
        if(!med_qa::contains(keys, key)){
            m_cacheM->removeItem(key);
        }else{
            printf("retain key: %s\n", key.data());
        }
    }
}
EDManager* EDManager::merge(EDManager& oth){
    using namespace _h7;
    if(m_cacheM == nullptr){
        fprintf(stderr, "EDManager::merge >> main is empty, no need.\n");
        return &oth;
    }
    if(oth.m_cacheM == nullptr){
        fprintf(stderr, "EDManager::merge >> oth is empty, no need.\n");
        return this;
    }
    auto names_main = m_cacheM->getItemNames();
    auto names_sub = oth.m_cacheM->getItemNames();
    //
    std::map<String, MergeItem> itemMap;
    //replace same item. from sub to main.
    for(int i = 0 ; i < (int)names_main.size() ; ++i){
        auto& s = names_main[i];
        MergeItem mi;
        mi.name = s;
        mi.idxMain = i;
        mi.idxSub = _h7::indexOf0(names_sub, s);
        itemMap.emplace(s, std::move(mi));
    }
    //add not exist item, from sub to main
    for(int i = 0 ; i < (int)names_sub.size() ; ++i){
        auto& s = names_sub[i];
        //not exist.
        if(itemMap.find(s) == itemMap.end()){
            MergeItem mi;
            mi.name = s;
            mi.idxMain = -1;
            mi.idxSub = i;
            itemMap.emplace(s, std::move(mi));
        }
    }
    auto it = itemMap.begin();
    while (it != itemMap.end()) {
        auto& item = it->second;
        if(item.idxMain >= 0){
            m_cacheM->removeItem(item.name);
        }
        String data;
        oth.m_cacheM->getItemData(item.name, data);
        m_cacheM->addItem(item.name, data);
        //
        ++it;
    }
    return this;
}
EDManager* EDManager::mergeByPriority(EDManager& oth, bool curIsHigh){
    if(curIsHigh){
        return merge(oth);
    }else{
        return oth.merge(*this);
    }
}
void EDManager::compressTo(CString encOutDesc){
    MED_ASSERT(m_cacheM);
    using namespace h7;
    auto desc = h7::utils::split(",", encOutDesc);
    MED_ASSERT(desc.size() == 3);
    h7::PerformanceHelper ph;
    ph.begin();
    m_cacheM->compressTo(desc[0], desc[1], desc[2]);
    ph.print("compressTo::" + encOutDesc);
}
void EDManager::compressTo(CString encOutDesc, CString salt){
    if(salt.empty()){
        compressTo(encOutDesc);
        return;
    }
    using namespace h7;
    h7::PerformanceHelper ph;
    ph.begin();
    MED_ASSERT(m_cacheM);
    auto newCM = std::unique_ptr<CacheManager>(new h7::CacheManager(2 << 30));
    int c = m_cacheM->getItemCount();
    for(int i = 0 ; i < c ; ++i){
    //for(int i :MFOREACH(c)){
        String name;
        String data;
        m_cacheM->getItemAt(i, name, data);
        //internal, no need enc
        if(name.find(__INTERNAL_PREFIX) != String::npos){
            newCM->addItem(name, data);
            continue;
        }
        //
        size_t len = data.length();
        char lenStr[sizeof(size_t)];
        memcpy(lenStr, &len, sizeof(size_t));
        String saltStr = salt;
        std::random_shuffle(saltStr.begin(), saltStr.end());
        auto cs = "1" + String(lenStr) + saltStr + data;
        std::reverse(cs.begin(), cs.end());
        newCM->addItem(name, cs);
    }
    newCM->addItem(__SALT_KEY, salt);
    //
    auto desc = h7::utils::split(",", encOutDesc);
    MED_ASSERT(desc.size() == 3);
    newCM->compressTo(desc[0], desc[1], desc[2]);
    ph.print("compressTo::" + encOutDesc);
}
//-------------
void EDManager::setIgnorePath(CString paths){
    m_ignorePath = h7::utils::split(",", paths);
}
void EDManager::setIncludePath(CString paths){
    m_incPaths = h7::utils::split(",", paths);
}
bool EDManager::shouldIgnore(CString path){
    if(m_ignorePath.empty()){
        return false;
    }
    for(auto& str : m_ignorePath){
        if(strings::StartsWith(path, str)){
            return true;
        }
    }
    return false;
}
bool EDManager::shouldInclude(CString path){
    if(m_incPaths.empty()){
        return true;
    }
    for(auto& str : m_incPaths){
        if(strings::StartsWith(path, str)){
            return true;
        }
    }
    return false;
}
String EDManager::getKPID(){
#ifdef DISABLE_ONNX2TRT
    CmdHelper2 ch("nvidia-smi -q | grep -i uuid");
    String _str;
    if(ch.runCmdDirectly(_str)){
        return _str;
    }
    return "";
#else
    return med::Recognizer::getGpuId();
#endif
}
void EDManager::prints(int limitLen){
    if(m_cacheM){
        int c = m_cacheM->getItemCount();
        printf("[EDM] prints: limit = %d, count = %d\n", limitLen, c);
        for(int i = 0 ; i < c; ++i){
        //for(int i :MFOREACH(c)){
            String name;
            String data;
            m_cacheM->getItemAt(i, name, data);
            if((int)data.length() > limitLen){
                auto hash = fasthash64(data.data(), data.length(), 11);
                printf("kv(over_limit) = %s, hash = %lu\n", name.data(), hash);
            }else{
                printf("kv = %s,%s\n", name.data(), data.data());
            }
        }
    }
}
void EDManager::checkKPID(){
    auto setkpid = getItem("__$(KPID)");
    if(!setkpid.empty()){
        auto kpid = med_qa::EDManager::getKPID();
        if(setkpid != kpid){
            fprintf(stderr, "[ERROR] KPID error.\n");
            abort();
        }
    }
}

