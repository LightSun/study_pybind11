#pragma once

#include <sstream>
#include <iomanip>
#include "core/src/base64.h"
#include "core/src/string_utils.hpp"
#include "core/src/_time.h"
#include "core/src/IKeyValue.h"
#include "core/src/hash.h"
#include "core/src/collections.h"
#include "core/src/FileUtils.h"
#include "snappy/snappy.h"

#define COMHASH_HASH_SEED 17

namespace h7 {

using String = std::string;
using CString = const std::string&;
template<typename T>
using List = std::vector<T>;

enum KAuthInfoType{
    KAuthInfoType_OPT,
    KAuthInfoType_ALL,
};
struct KAuthInfo{
    unsigned long long preHash {0};
    int type {KAuthInfoType_OPT};
    int count {0};
    String text;

    String toString()const{
        return std::to_string(preHash) + ";" + std::to_string(type)
                + ";" + std::to_string(count)
                + ";" + text;
    }
    void parse(CString str){
        auto strs = h7::utils::split(";", str);
        MED_ASSERT_X(strs.size() == 4, "KAuthInfo::parse << " << str);
        preHash = h7::utils::getULong(strs[0]);
        type = h7::utils::getInt(strs[1]);
        count = h7::utils::getInt(strs[2]);
        text = strs[3];
    }
    bool equals(const KAuthInfo& o) const{
        if(preHash != o.preHash){
            return false;
        }
        if(type != o.type){
            return false;
        }
        if(count != o.count){
            return false;
        }
        if(text != o.text){
            return false;
        }
        return true;
    }
};

using CKAuthInfo = const KAuthInfo&;

static inline std::string formatNumber4(int num) {
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << num;
    return oss.str();
}
static inline std::string kencode(CKAuthInfo info){
    auto timeStr = std::to_string(h7_handler_os::getCurTime());
    String preStr = formatNumber4(timeStr.length());

    String str1 = preStr + info.toString() + timeStr;
    auto newStr = base64_encode(str1);
    String outStr;
    snappy::Compress(newStr.data(), newStr.length(), &outStr);
    return outStr;
}
static inline std::string kencode(CString str){
    KAuthInfo ai;
    ai.text = str;
    return kencode(ai);
}

static inline bool kdecode(CString str, KAuthInfo& out){
    String decStr;
    if(!snappy::Uncompress(str.data(), str.length(), &decStr)){
        return false;
    }
    auto kstr = base64_decode(decStr);
    auto lstr = kstr.substr(0, 4);
    auto timeLen = h7::utils::getInt(lstr);
    auto actLen = kstr.length() - 4 - timeLen;
    auto astr = kstr.substr(4, actLen);
    out.parse(astr);
    return true;
}

static inline std::string kencode2(CString str){
    String str2 = str;
    std::reverse(str2.begin(), str2.end());
    auto timeStr = std::to_string(h7_handler_os::getCurTime());
    String preTimeStr = formatNumber4(timeStr.length());
    String preStr2 = formatNumber4(str2.length());
    //
    String str1 = str2 + preStr2 + timeStr + preTimeStr;
    auto newStr = base64_encode(str1);
    String outStr;
    snappy::Compress(newStr.data(), newStr.length(), &outStr);
    return outStr + preTimeStr;
}
static inline bool kdecode2(CString _str,String& out){
    String str = _str;
    str = str.substr(0, str.length() - 4);
    String decStr;
    if(!snappy::Uncompress(str.data(), str.length(), &decStr)){
        return false;
    }
    auto str1 = base64_decode(decStr);
    int totalLen = str1.length();
    //12345
    auto tLen = h7::utils::getUInt(str1.substr(totalLen - 4, 4));
    str1 = str1.substr(0, totalLen - 4 - tLen);
    //
    totalLen = str1.length();
    tLen = h7::utils::getUInt(str1.substr(totalLen - 4, 4));
    auto str2 = str1.substr(0, tLen);
    std::reverse(str2.begin(), str2.end());
    out = str2;
    return true;
}
static inline std::string kencode2(KAuthInfo& oinfo, CString extStr){
    auto enc2 = kencode2(extStr);
    auto enc2Post = formatNumber4(enc2.length());
    String encTextStr;
    snappy::Compress(oinfo.text.data(), oinfo.text.length(), &encTextStr);
    oinfo.text = enc2Post + encTextStr + enc2;
    oinfo.text = base64_encode(oinfo.text);
    return h7::kencode(oinfo);
}
static inline bool kdecode2(CString str, KAuthInfo& oinfo, String& extStr){
    if(!h7::kdecode(str, oinfo)){
        return false;
    }
    auto text = base64_decode(oinfo.text);
    auto enc2Len = h7::utils::getUInt(text.substr(0, 4));
    text = text.substr(4);
    int leftLen = text.length() - enc2Len;
    auto enc2 = text.substr(text.length() - enc2Len, enc2Len);
    text = text.substr(0, leftLen);
    oinfo.text = "";
    if(!snappy::Uncompress(text.data(), text.length(), &oinfo.text)){
        return false;
    }
    if(kdecode2(enc2, extStr)){
        return true;
    }
    return false;
}

//---------------
struct ComHashInput{
    using uint64 = unsigned long long;
    List<String> preBuildKeys;
    List<String> optHardwareKeys;
    List<String> hardwareKeys;
    String optHardwareKeysFile;
    String outFile;            //used for 1 and 2
    //
    String mode;  //for only auth1
    //input2
    String input1Content;        //cfg: input1File
    String postInput1Keys;       //split with ','

    void init(KVHelper& helper){
        helper.getVector("preBuildKeys", preBuildKeys);
        helper.getVector("optHardwareKeys", optHardwareKeys);
        helper.getVector("hardwareKeys", hardwareKeys);
        optHardwareKeysFile = helper.getString("optHardwareKeysFile");
        outFile = helper.getString("outFile");
        mode = helper.getString("mode");
        //
        auto input1File = helper.getString("input1File");
        if(!input1File.empty()){
            input1Content = h7::FileUtils::getFileContent(input1File);
        }
        postInput1Keys = helper.getString("postInput1Keys");
        if(postInput1Keys.empty()){
            auto f1 = helper.getString("postInput1KeysFile");
            if(!f1.empty() && h7::FileUtils::isFileExists(f1)){
                printf("postInput1KeysFile >> exist.\n");
                postInput1Keys = h7::FileUtils::getFileContent(f1);
            }
        }
    }
    String genAuth1(){
        uint64 hash = COMHASH_HASH_SEED;
        {
            for(auto& k : preBuildKeys){
                hash = fasthash64(k.data(), k.length(), hash);
            }
        }
        h7::KAuthInfo authInfo;
        authInfo.preHash = hash;
        if(hardwareKeys.empty()){
            if(optHardwareKeys.empty()){
                String fc;
                if(!optHardwareKeysFile.empty()){
                    fc = h7::FileUtils::getFileContent(optHardwareKeysFile);
                }
                if(fc.empty()){
                    fprintf(stderr, "one of optHardwareKeys/hardwareKeys/optHardwareKeysFile must exist.\n");
                    return "";
                }
                optHardwareKeys = h7::utils::split(",", fc);
            }
            std::vector<String> hashList;
            int size = optHardwareKeys.size();
            for(int i = 0 ; i < size ; ++i){
                auto& k = optHardwareKeys[i];
                auto hash0 = fasthash64(k.data(), k.length(), hash);
                hashList.push_back(std::to_string(hash0));
            }
            auto hashStr = med_qa::concatStr(hashList, ",");
            authInfo.text = hashStr;
            authInfo.type = h7::KAuthInfoType_OPT;
            authInfo.count = hashList.size();
        }else{
            for(auto& k : hardwareKeys){
                hash = fasthash64(k.data(), k.length(), hash);
            }
            auto hashStr = std::to_string(hash);
            authInfo.text = hashStr;
            authInfo.type = h7::KAuthInfoType_ALL;
            authInfo.count = hardwareKeys.size();
        }
        auto finalContent = h7::kencode(authInfo);
        {
            h7::KAuthInfo oth;
            MED_ASSERT(h7::kdecode(finalContent, oth));
            MED_ASSERT(oth.equals(authInfo));
        }
        return finalContent;
    }
    //--input1File xxx
    //postInput1KeysFile(opt)
    String genAuth2(){
        if(postInput1Keys.empty()){
            fprintf(stderr, "[WARN] for auth2: postInput1Keys/postInput1KeysFile not exist, will use default\n");
        }
        String keyStr;
        {
            uint64 _hash;
            if(postInput1Keys.empty()){
                _hash = COMHASH_HASH_SEED;
            }else{
                _hash = fasthash64(postInput1Keys.data(),
                                   postInput1Keys.length(), COMHASH_HASH_SEED);
            }
            keyStr.append(std::to_string(_hash));
            keyStr.append(",Med@" + h7_handler_os::getCurTimeStr());
        }
        KAuthInfo oinfo;
        MED_ASSERT(h7::kdecode(input1Content, oinfo));
        String cs;
        {
            auto oinfo2 = oinfo;
            cs = kencode2(oinfo2, keyStr);
        }
        {
            h7::KAuthInfo oth;
            String extStr;
            MED_ASSERT(h7::kdecode2(cs, oth, extStr));
            MED_ASSERT(oth.equals(oinfo));
            MED_ASSERT(extStr == keyStr);
        }
        return cs;
    }
};

struct ComHashGen{

    ComHashInput m_input;

    ComHashGen(IKeyValue* kv){
        KVHelper helper(kv);
        m_input.init(helper);
    }
    ComHashGen(const ComHashInput& in):m_input(in){}

    int genAuth1(String* outStr = nullptr){
        auto cs = m_input.genAuth1();
        if(cs.empty()){
            return 1;
        }
        doOut(cs);
        if(outStr){
            *outStr = cs;
        }
        return 0;
    }
    int genAuth2(){
        auto cs = m_input.genAuth2();
        if(cs.empty()){
            return 1;
        }
        doOut(cs);
        return 0;
    }
    void doOut(CString finalContent){
        if(m_input.outFile.empty()){
            printf("%s\n", finalContent.data());
        }else{
            h7::FileUtils::writeFile(m_input.outFile, finalContent);
        }
    }
};

}
