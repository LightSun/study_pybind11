#pragma once

#include "core/src/FileUtils.h"
#include "core/src/string_utils.hpp"
#include "core/src/Prop.h"
#include "core/src/collections.h"

namespace h7_app {

using String = std::string;
using CString = const std::string&;
template<typename T>
using List = std::vector<T>;

enum{
    kRTPreConfig_V1 = 1,
    kRTPreConfig_V2,   //for QA
};

//__DirKeys__: def check value of keys, which contains "/../"
//__OneKeyDir__: base dir of OneKey-install
struct RTPreConfig{
    bool bertNeed {false};
    bool bertLibPathNeed {false};
    bool bertFilterFileNeed {false};
    bool hsyConfigFileNeed {false};
    bool ocrNeed {false}; //OcrV4_desc

    int version {kRTPreConfig_V1};
    List<String> dirKeys;

    void readFromProp(CString file, CString oneKeyDir){
        if(h7::FileUtils::isFileExists(file)){
            med_qa::Prop prop;
            prop.putString("__OneKeyDir__", oneKeyDir);
            //
            prop.load(file);
            processDirValue(prop);
            //
            h7::KVHelper kvh(&prop);
            read(kvh);
        }else{
            printf("readFromProp >>file not exist. '%s'\n", file.data());
        }
    }

    void read(h7::KVHelper& helper){
        bertNeed = helper.getBool("bertNeed");
        bertLibPathNeed = helper.getBool("bertLibPathNeed");
        hsyConfigFileNeed = helper.getBool("hsyConfigFileNeed");
        bertFilterFileNeed = helper.getBool("bertFilterFileNeed");
        ocrNeed = helper.getBool("ocrNeed");
    }
private:
    void processDirValue(med_qa::Prop& prop){
        List<String> keys;
        h7::KVHelper kvh(&prop);
        kvh.getVector("__DirKeys__", keys);
        version = kvh.getInt("version", version);
        if(keys.empty() && version > kRTPreConfig_V1){
            auto keys2 = prop.keys();
            keys = {keys2.begin(), keys2.end()};
        }
        for(auto& k : keys){
            processTargetValue0(prop, k);
        }
    }
    void processTargetValue0(med_qa::Prop& prop, CString key){
        auto mdDir = prop.getString(key);
        if(!mdDir.empty()){
            bool changed = false;
            while (true) {
                int pos = mdDir.find("/../");
                if(pos >= 0){
                    auto suffix = mdDir.substr(pos + 3);
                    auto prefix = mdDir.substr(0, pos);
                    //  /a/b/123/../a
                    auto idx = prefix.rfind("/");
                    if(idx >= 0){
                        auto actPrefix = prefix.substr(0, idx);
                        mdDir = actPrefix + suffix;
                        changed = true;
                    }else{
                        break;
                    }
                }else{
                    break;
                }
            }
            if(changed){
                prop.putString(key, mdDir);
            }
        }
    }
};

struct RTConfig{
    String edmFiles;
    String edmPtFiles;
    String cacheDir;
    String hsyConfigFile;
    String hsyCacheDir;
    //
    String bertLibPath;
    String bertFilterFile;
    String bertVocabPath;
    String bertCtcPath;
    String bertCache;
    //
    String ocrDescDir;

    static void genFile(CString rootDir, const List<String>& ldDirs,
                 const RTPreConfig& pcfg, CString rtDstFile){
        RTConfig rtcfg;
        auto rtVec = rtcfg.gen(rootDir, ldDirs, pcfg);
        auto rtCS = med_qa::concatStr(rtVec, NEW_LINE);
        h7::FileUtils::writeFile(rtDstFile, rtCS);
        printf("write rtDstFile end. file = '%s'\n", rtDstFile.data());
    }
    //hsyConfigFileNeed: /config/aisdk/config.yaml
    //bertNeed: res/bert/new_vocab_0207_new.txt
    //bertNeed: res/bert/ctc_correct_tags_0313.txt
    //      bertFilterFileNeed: res/bert/filter.txt
    //      bertLibPathNeed: libnlp_bert.so/libnlp_bert_V<num>.so
    //ocrNeed: /res/module/OcrV4_desc
    List<String> gen(CString rootDir, const List<String>& ldDirs,
                     const RTPreConfig& pcfg){
        String mainDir = rootDir + "/main";
        List<String> retList;
        processEDM(rootDir);
        //
        cacheDir = mainDir + "/cache";
        h7::FileUtils::mkdirs(cacheDir);
        if(pcfg.hsyConfigFileNeed){
            hsyConfigFile = mainDir + "/config/aisdk/config.yaml";
            MED_ASSERT_X(h7::FileUtils::isFileExists(hsyConfigFile),
                         "File not exist. " << hsyConfigFile);
            hsyCacheDir = mainDir + "/cache/hsy";
            h7::FileUtils::mkdirs(hsyCacheDir);
        }
        if(pcfg.bertNeed){
            bertVocabPath = mainDir + "/res/bert/new_vocab_0207_new.txt";
            bertCtcPath = mainDir + "/res/bert/ctc_correct_tags_0313.txt";
            MED_ASSERT_X(h7::FileUtils::isFileExists(bertVocabPath),
                         "File not exist. " << bertVocabPath);
            MED_ASSERT_X(h7::FileUtils::isFileExists(bertCtcPath),
                         "File not exist. " << bertCtcPath);
            bertCache = mainDir + "/cache/bert";
            h7::FileUtils::mkdirs(bertCache);
            if(pcfg.bertFilterFileNeed){
                bertFilterFile = mainDir + "/res/bert/filter.txt";
            }
            if(pcfg.bertLibPathNeed){
                //search file on libDir.
                String dstF;
                bool found = false;
                for(auto& ldir : ldDirs){
                    //libnlp_bert_V<num>.so
                    dstF = ldir + "/libnlp_bert.so";
                    if(h7::FileUtils::isFileExists(dstF)){
                        found = true;
                        break;
                    }
                }
                //
                for(auto& ldir : ldDirs){
                    //libnlp_bert_V<num>.so
                    for(int i = 3 ; i < 6; ++i){
                        dstF = ldir + "/libnlp_bert_V" + std::to_string(i) + ".so";
                        if(h7::FileUtils::isFileExists(dstF)){
                            found = true;
                            break;
                        }
                    }
                }
                MED_ASSERT_X(found, "[ERROR] bertLibPathNeed >> can't find libnlp_bert.so/libnlp_bert_V<num>.so in lib dirs.");                bertLibPath = dstF;
                bertLibPath = dstF;
            }
        }
        if(pcfg.ocrNeed){
            ocrDescDir = mainDir + "/res/module/OcrV4_desc";
            MED_ASSERT_X(h7::FileUtils::isDirReadable(ocrDescDir),
                         "File not exist. " << ocrDescDir);
        }
        //
        retList.push_back("edmFiles=" + edmFiles);
        if(!edmPtFiles.empty()){
            retList.push_back("edmPtFiles=" + edmPtFiles);
        }
        retList.push_back("cacheDir=" + cacheDir);
        if(!hsyConfigFile.empty()){
            retList.push_back("hsyConfigFile=" + hsyConfigFile);
        }
        if(!hsyCacheDir.empty()){
            retList.push_back("hsyCacheDir=" + hsyCacheDir);
        }
        if(!bertLibPath.empty()){
            retList.push_back("bertLibPath=" + bertLibPath);
        }
        if(!bertFilterFile.empty()){
            retList.push_back("bertFilterFile=" + bertFilterFile);
        }
        if(!bertVocabPath.empty()){
            retList.push_back("bertVocabPath=" + bertVocabPath);
        }
        if(!bertCtcPath.empty()){
            retList.push_back("bertCtcPath=" + bertCtcPath);
        }
        if(!bertCache.empty()){
            retList.push_back("bertCache=" + bertCache);
        }
        if(!ocrDescDir.empty()){
            retList.push_back("ocrDescDir=" + ocrDescDir);
        }
        return retList;
    }
private:
    void processEDM(CString rootDir){
        //record,data
        //edmFiles
        auto modDir = rootDir + "/main/res/module";
        {
            List<String> cvDescs;
            auto dstDir = modDir + "/trtenc_desc";
            if(h7::FileUtils::isDirReadable(dstDir)){
                cvDescs.push_back(dstDir);
            }
            for(int i = 1 ; ; ++i){
                dstDir = modDir + "/trtenc_desc" + std::to_string(i);
                if(h7::FileUtils::isDirReadable(dstDir)){
                    cvDescs.push_back(dstDir);
                }else{
                    break;
                }
            }
            edmFiles = med_qa::concatStr(cvDescs, ",");
        }
        //edmPtFiles
        {
            List<String> ptDescs;
            auto dstDir = modDir + "/bert_desc";
            if(h7::FileUtils::isDirReadable(dstDir)){
                ptDescs.push_back(dstDir);
            }
            dstDir = modDir + "/nlp_desc";
            if(h7::FileUtils::isDirReadable(dstDir)){
                ptDescs.push_back(dstDir);
            }
            for(int i = 1 ; ; ++i){
                dstDir = modDir + "/nlp_desc" + std::to_string(i);
                if(h7::FileUtils::isDirReadable(dstDir)){
                    ptDescs.push_back(dstDir);
                }else{
                    break;
                }
            }
            edmPtFiles = med_qa::concatStr(ptDescs, ",");
        }
    }
};

}
