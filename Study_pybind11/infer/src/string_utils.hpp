#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <vector>
#include <regex>
#include <algorithm>
#include <iostream>
#include "ocr_common.h"

namespace h7 {
namespace utils{

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

inline std::string& trim(std::string &s) {
    if (s.empty()) {
        return s;
    }
    s.erase(0, s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
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

}

}

#endif // STRING_UTILS_H
