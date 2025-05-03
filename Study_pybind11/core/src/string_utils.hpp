#pragma once

#include <vector>
#include <regex>
#include <algorithm>
#include <iostream>
#include "core/src/common.h"
//#include "utils/Regex.h"
//#include "include/ctre.hpp"

namespace h7 {
namespace utils{

static inline String formatFloat(float v, CString fmt){
    char buf[64];
    snprintf(buf, 64, fmt.data(), v);
    return String(buf);
}

static inline float getFloat(CString str, float def = 0){
    if(str.empty() || str == "-"){
        return def;
    }
    try{
           return std::stof(str);
       }catch (std::invalid_argument&){
           //LOGW("sxv4 _getFLoat >> Invalid_argument('%s').\n",str.data());
       }catch (std::out_of_range&){
           //LOGW("sxv4 _getFLoat >> Out of range('%s').\n",str.data());
       }catch (...) {
           //LOGW("sxv4 _getFLoat >> Something else('%s').\n",str.data());
       }
    return def;
}

static inline int getInt(CString str, int def = 0, bool log = false){
    if(str.empty() || str == "-"){
        return def;
    }
    try{
           return std::stoi(str);
       }catch (std::invalid_argument&){
        if(log){
            LOGW("sxv4 getInt >> Invalid_argument('%s').\n",str.data());
        }
       }catch (std::out_of_range&){
        if(log)
             LOGW("sxv4 getInt >> out_of_range('%s').\n",str.data());
       }catch (...) {
        if(log)
            LOGW("sxv4 getInt >> wrong('%s').\n",str.data());
       }
    return def;
}
static inline unsigned int getUInt(CString str, unsigned int def = 0){
    if(str.empty() || str == "-"){
        return def;
    }
    try{
           return std::stoul(str);
       }catch (std::invalid_argument&){
       }catch (std::out_of_range&){
       }catch (...) {
       }
    return def;
}
static inline long long getLong(CString str, long long def = 0){
    if(str.empty() || str == "-"){
        return def;
    }
    try{
           return std::stoll(str);
       }catch (std::invalid_argument&){
       }catch (std::out_of_range&){
       }catch (...) {
       }
    return def;
}

static inline long long getULong(CString str, long long def = 0){
    if(str.empty() || str == "-"){
        return def;
    }
    try{
           return std::stoull(str);
       }catch (std::invalid_argument&){
       }catch (std::out_of_range&){
       }catch (...) {
       }
    return def;
}

inline String newLineStr(){
#if defined(_WIN32) || defined(WIN32)
    return "\r\n";
#else
    return "\n";
#endif
}

//start: include.
//end: exclude
inline String subString(CString str,int start, int end){
    return str.substr(start, end - start);
}

inline bool startsWith(CString s, CString sub){
    return s.find(sub)==0;
}

inline bool endsWith(CString s,CString sub){
    return s.rfind(sub)==(s.length()-sub.length());
}

inline bool rGrepl(CString pat, CString text){
    std::regex reg(pat.c_str());
    std::cmatch m;
    return std::regex_search(text.c_str(), m, reg);
}

inline bool rGrepl(std::regex& reg, CString text){
    std::cmatch m;
    return std::regex_search(text.c_str(), m, reg);
}

inline bool kMatch(CString pat, CString text){
    std::regex reg(pat);
    std::smatch m;
    return std::regex_match(text, m, reg);
}

inline String rGsub(CString pat, CString replace, CString text){
    std::regex reg(pat.c_str());
    return std::regex_replace(text.c_str(), reg, replace.c_str());
}

inline bool match(std::regex& reg, CString text){
    return rGrepl(reg, text);
}

inline String replace(CString pat, CString replace, CString text){
    std::regex reg(pat.c_str());
    return std::regex_replace(text.c_str(), reg, replace.c_str());
}

inline bool matchAll(CString pat, std::vector<String>& vec){
    std::regex reg(pat.c_str());
    int size = vec.size();
    for(int i = 0 ; i < size ; i ++){
        if(!rGrepl(reg, vec[i])){
            return false;
        }
    }
    return true;
}

inline void extractStr(CString pat, CString text, std::vector<String>& out){
    std::regex reg(pat.c_str());
    std::smatch result;
    std::string::const_iterator iterStart = text.begin();
    std::string::const_iterator iterEnd = text.end();
    while (std::regex_search(iterStart, iterEnd, result, reg))
    {
        out.push_back(result[0]);
        iterStart = result[0].second;//search left str
    }
}
inline std::vector<String> split(CString pat, CString text){

    std::regex reg(pat);
    std::sregex_token_iterator first{text.begin(),
                               text.end(), reg, -1}, last;
    return {first, last};
}

inline void split(CString pat, CString text, std::vector<String>& out){
    std::regex reg(pat);
    std::sregex_token_iterator first{text.begin(),
                               text.end(), reg, -1}, last;
    out.assign(first, last);
}

inline std::vector<std::string> split2(CString pat, const std::string& str)
{
    std::vector<std::string> result;
    std::vector<int> poss;
    const int patLen = pat.length();
    for(int i = 0 ;;){
        int pos = str.find(pat, i);
        if(pos >= 0){
            poss.push_back(pos);
            i = pos + patLen;
        }else{
            break;
        }
    }
    if(poss.empty()){
        return {str};
    }
    if(poss[0] > 0){
        result.push_back(str.substr(0, poss[0]));
    }else{
        result.push_back("");
    }
    if(poss.size() >= 2){
        for(int i = 0 ; i < (int)poss.size() - 1 ; ++i){
            int p1 = poss[i];
            int p2 = poss[i + 1];
            String str2 = str.substr(p1 + patLen, p2 - p1 - 1);
            result.push_back(str2);
        }
    }
    if(poss[(int)poss.size()-1] < (int)str.length() - 1){
        result.push_back(str.substr(poss[poss.size()-1] + 1));
    }else{
        result.push_back("");
    }
    return result;
}

inline std::string& trim(std::string &s) {
    if (s.empty()) {
        return s;
    }
    s.erase(0, s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
    return s;
}
inline std::string& removeNewLine(std::string &s) {
    if (s.empty()) {
        return s;
    }
    int pos;
    while (true) {
        pos = s.find("\n");
        if(pos >= 0){
            if(pos > 0 && s.data()[pos - 1] =='\r'){
                s = s.replace(pos - 1, 2, "");
            }else{
                s = s.replace(pos, 1, "");
            }
        }else{
            break;
        }
    }
    return s;
}
inline std::string& trimTailZero(std::string &s) {
    if (s.empty() || s == "0") {
        return s;
    }
    s.erase(s.find_last_not_of("0") + 1);
    if(endsWith(s, ".")){
        s.erase(s.length()-1);
    }
    return s;
}

inline bool isNumber(CString str){
    return str.find_first_not_of("0123456789") == std::string::npos;
}
inline bool isDecimal(CString str){
    return str.find_first_not_of("0123456789.") == std::string::npos;
}

inline bool trimLastR(String& str){
    if(str.length() > 0 && str.c_str()[str.length()-1]=='\r'){
        str.erase(str.length()-1);
        return true;
    }
    return false;
}

inline std::vector<char> string2vec(const std::string& str){
    std::vector<char> vec;
    vec.resize(str.length());
    memcpy(vec.data(), str.data(), str.length());
    return vec;
}

inline void toUpper(std::vector<String>& vec){
    int size = vec.size() ;
    for(int i = 0 ; i < size ; ++i){
        String& str = vec[i];
        std::transform(str.begin(),str.end(),str.begin(),::toupper);
    }
}

inline void toLower(std::vector<String>& vec){
    int size = vec.size() ;
    for(int i = 0 ; i < size ; ++i){
        String& str = vec[i];
        std::transform(str.begin(),str.end(),str.begin(),::tolower);
    }
}

inline String preprocessSpecialChars(CString s){
    //for linux cmd: we need process the special chars
    //output-ds\ \(5\)~\!@#\$%\^\&\*_+\=-.vcf.gz
    std::vector<char> src_strs = {' ','(',')','!','$','^','&','*','=',
                                  ',' ,'\'',';', '?', '|'
                                 };
    std::vector<String> dst_strs = {"\\ ","\\(","\\)","\\!","\\$","\\^","\\&","\\*","\\=",
                                    "\\,", "\\'", "\\;","\\?", "\\|"
                                 };
    MED_ASSERT(src_strs.size() == dst_strs.size());
    String ret;
    ret.reserve(256);
    int ssize = src_strs.size();
    int size = s.length();
    char* data = (char*)&s[0];
    for(int i = 0 ; i < size ; ++i){
        auto& c = data[i];
        bool handled = false;
        for(int j = 0 ; j < ssize; ++j){
            if(c == src_strs[j]){
                handled = true;
                ret += dst_strs[j];
                break;
            }
        }
        if(!handled){
            ret += String(1, c);
        }
    }
    return ret;
}

static inline std::vector<String> getNvGpuIds(CString cs){
    std::vector<String> gpuIds;
    auto strs = split("( )+", cs);
    String pat = "[a-zA-Z0-9\\-]+";
    for(size_t i = 0 ; i < strs.size() ; ++i){
        auto& s = strs[i];
        if(s == "UUID"){
            if(i + 2 < strs.size()){
                std::vector<String> vec;
                extractStr(pat, strs[i + 2], vec);
                if(!vec.empty()){
                    gpuIds.push_back(vec[0]);
                }
            }
            i += 2;
        }
    }
    return gpuIds;
}

static inline std::vector<String> getHwGpuIds(CString cs){
    std::vector<String> gpuIds;
    auto strs = split("( )+", cs);
    String pat = "[a-zA-Z0-9\\-]+";
    for(size_t i = 0 ; i < strs.size() ; ++i){
        auto& s = strs[i];
        if(s == "Number"){
            if(i + 2 < strs.size()){
                std::vector<String> vec;
                extractStr(pat, strs[i + 2], vec);
                if(!vec.empty()){
                    gpuIds.push_back(vec[0]);
                }
            }
            i += 2;
        }
    }
    return gpuIds;
}

}

}
