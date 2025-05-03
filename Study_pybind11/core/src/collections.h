#pragma once

#include <vector>
#include <set>
#include <unordered_map>
#include <functional>
#include "core/src/common.h"

namespace med_qa {

template <typename T>
std::string concat(const std::vector<T>& vec, const T& sep){
    std::string ret;
    ret.reserve(4096);
    const int size = vec.size();
    for(int i = 0 ; i < size ; ++i){
        ret.append(std::to_string(vec[i]));
        if( i != size - 1){
            ret.append(std::to_string(sep));
        }
    }
    return ret;
}
static inline std::string concatStr(const std::vector<std::string>& vec, const std::string& sep){
    std::string ret;
    ret.reserve(4096);
    const int size = vec.size();
    for(int i = 0 ; i < size ; ++i){
        ret.append(vec[i]);
        if( i != size - 1){
            ret.append(sep);
        }
    }
    return ret;
}

template <typename T>
std::vector<T> merge(const std::vector<std::vector<T>>& vec, bool removeDup){
    std::vector<T> ret;
    if(removeDup){
        std::set<std::vector<T>> set(vec.begin(), vec.end());
        auto it = set.begin();
        for(;it != set.end(); ++it){
            ret.insert(ret.end(), it->begin(), it->end());
        }
    }else{
        const int size = vec.size();
        for(int i = 0 ; i < size ; ++i){
            ret.insert(ret.end(), vec[i].begin(), vec[i].end());
        }
    }
    return ret;
}

template <typename T,typename R>
std::vector<R> extract(const std::vector<std::vector<T>>& vec,
                       std::function<R(const std::vector<T>&)> func){

    std::vector<R> ret;
    const int size = vec.size();
    for(int i = 0 ; i < size ; ++i){
        ret.push_back(func(vec[i]));
    }
    return ret;
}

template <typename T>
T avg(const std::vector<T>& vec){
    T sum = 0;
    const int size = vec.size();
    for(int i = 0 ; i < size ; ++i){
        sum += vec[i];
    }
    return size > 0 ? sum / size : 0;
}

template <typename T>
bool contains(const std::vector<T>& vec, const T& t){
    const int size = vec.size();
    for(int i = 0 ; i < size ; ++i){
        if(vec[i] == t) return true;
    }
    return false;
}

template <typename T>
T findMaxTimes(const std::vector<T>& vec){
    std::unordered_map<T, int> cntMap;
    int size = vec.size();
    for(int i = 0 ; i < size ; ++i){
        auto it = cntMap.find(vec[i]);
        if(it != cntMap.end()){
            it->second ++;
        }else{
            cntMap[vec[i]] = 1;
        }
    }
    auto it = cntMap.begin();
    int maxCnt = 0;
    T key;
    for(;it != cntMap.end();++it){
        if(it->second > maxCnt){
            key = it->first;
            maxCnt = it->second;
        }
    }
    MED_ASSERT(maxCnt > 0);
    return key;
}

static inline std::vector<float> generateProbs(int label, float prob, int len){
    std::vector<float> l(len, 0);
    l[label + 1] = prob;
    l[0] = label + 1;
    return l;
}

}
