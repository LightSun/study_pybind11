#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include "core/src/locks.h"

namespace h7 {
//string, cacheItem
template<typename K, typename V, class Lock = NullLock>
class FixedOrderMap{
    typedef Lock Lock_type;
    using Guard = std::lock_guard<Lock_type>;
    using Pair = std::pair<K,V>;
public:
    FixedOrderMap(int size){
        m_vec.resize(size);
    }

    int getTotaoCapcity()const{ return m_vec.size();}
    int getUsedCapcity(){return m_usedIdx + 1;}

    V* requireItem(const K& k){
        Guard g(m_lock);
        if(m_usedIdx < m_vec.size() - 1){
            ++m_usedIdx;
        }else{
            m_usedIdx = 0;
            //remove 0
            m_map.erase(m_vec[0].first);
        }
        auto& item = m_vec[m_usedIdx];
        item.first = k;
        m_map[k] = &item;
        return &item.second;
    }
    V* getItem(const K& k){
        Guard g(m_lock);
        auto it = m_map.find(k);
        if(it != m_map.end()){
            return &it->second->second;
        }
        return nullptr;
    }
    void visitItems(std::function<void(int, std::pair<K,V>* p)> func){
        Guard g(m_lock);
        func(m_usedIdx + 1, nullptr);
        for(int i = 0 ; i <= m_usedIdx ; ++i){
            auto& item = m_vec[i];
            func(i, &item);
        }
    }
    long long memorySize(std::function<long long(V& p)> func){
        long long total = 0;
        Guard g(m_lock);
        int size = m_vec.size();
        for(int i = 0 ; i < size ; ++i){
            total += m_vec[i].first.size();
            total += func(m_vec[i].second);
        }
        return total;
    }
private:
    int m_usedIdx {-1};
    std::vector<Pair> m_vec;
    std::unordered_map<K,Pair*> m_map;
    mutable Lock m_lock;
};
}
