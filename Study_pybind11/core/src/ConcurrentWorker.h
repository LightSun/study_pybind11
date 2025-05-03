#pragma once

#include <functional>
#include <thread>
#include "core/src/SaveQueue.h"
#include "core/src/locks.h"

namespace h7 {

template <typename T>
class ConcurrentWorker
{
public:
    ConcurrentWorker(int count, std::function<void(T& task)> func):m_func(func){
        m_queue = std::unique_ptr<SaveQueue<T>>(
            new SaveQueue<T>(tableSizeFor(count)));
    }
    ~ConcurrentWorker(){
        stop();
    }

    void start(){
        bool exp = false;
        if(m_started.compare_exchange_strong(exp, true)){
            std::thread thd([this](){
                while (!m_reqStop.load()) {
                    T task;
                    if(m_queue->dequeue(task)){
                        m_func(task);
                    }
                }
                m_lock.notify();
            });
            thd.detach();
        }
    }
    void stop(){
        if(m_started.load()){
            bool exp = false;
            if(m_reqStop.compare_exchange_strong(exp, true)){
                m_lock.wait();
            }
        }
    }
    void addTask(const T& t){
        for (;;) {
            if(m_queue->enqueue(t)){
                break;
            }
        }
    }

public:
    //>=N. with 2^n
    static int tableSizeFor(int cap) {
        const int MAXIMUM_CAPACITY = 1073741824;
        int n = cap - 1;
        n |= ((unsigned int)n >> 1);
        n |= ((unsigned int)n >> 2);
        n |= ((unsigned int)n >> 4);
        n |= ((unsigned int)n >> 8);
        n |= ((unsigned int)n >> 16);
        return (n < 0) ? 1 : (n >= MAXIMUM_CAPACITY) ? MAXIMUM_CAPACITY : n + 1;
    }

private:
    std::unique_ptr<SaveQueue<T>> m_queue;
    std::function<void(T& task)> m_func;
    MutexLock m_lock;
    std::atomic_bool m_reqStop {false};
    std::atomic_bool m_started {false};
};

template <typename R,typename T,typename P>
class ConcurrentCallbackWorker{
public:
    using Callback = std::function<void(const R& r)>;
    struct Item{
        T t;
        P p;
        Callback cb;
    };
    using SItem = std::shared_ptr<Item>;
    ConcurrentCallbackWorker(int count, std::function<R(T& task, const P& p)> func)
        : m_func(func){
        int size = ConcurrentWorker<int>::tableSizeFor(count);
        m_queue = std::unique_ptr<SItem>(new SItem(size));
    }
    void start(){
        bool exp = false;
        if(m_started.compare_exchange_strong(exp, true)){
            std::thread thd([this](){
                while (!m_reqStop.load()) {
                    SItem task;
                    if(m_queue->dequeue(task)){
                        auto r = m_func(task->t, task->p);
                        task->cb(r);
                    }
                }
                m_lock.notify();
            });
            thd.detach();
        }
    }

    void stop(){
        if(m_started.load()){
            bool exp = false;
            if(m_reqStop.compare_exchange_strong(exp, true)){
                m_lock.wait();
            }
        }
    }
    void addTask(const T& t, const P& p, std::function<void(const R& r)> cb){
        SItem item = std::make_shared<Item>();
        item->t = t;
        item->p = p;
        item->cb = cb;
        addTask(item);
    }

    void addTask(const T& t, std::function<void(const R& r)> cb){
        SItem item = std::make_shared<Item>();
        item->t = t;
        item->cb = cb;
        addTask(item);
    }

    void addTask(SItem item){
        for (;;) {
            if(m_queue->enqueue(item)){
                break;
            }
        }
    }

private:
     std::function<R(T& task, const P& p)> m_func;
     std::unique_ptr<SaveQueue<SItem>> m_queue;
     MutexLock m_lock;
     std::atomic_bool m_reqStop {false};
     std::atomic_bool m_started {false};
};

}
