#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>

namespace h7 {

class NullLock {
 public:
  void lock() {}
  void unlock() {}
  bool try_lock() { return true; }
};

class CppLock {
 public:
  void lock() {
    while (flag.test_and_set(std::memory_order_acquire)) {}
  }

  void unlock() {
    flag.clear(std::memory_order_release);
  }
 private:
  std::atomic_flag flag {0};
};

class MutexLock {
 public:
  void lock() {
     mutex.lock();
  }
  void unlock() {
     mutex.unlock();
  }
  void wait(){
    std::unique_lock<std::mutex> lck(mutex);
    conv.wait(lck);
  }
  void notify(bool all = true){
    std::unique_lock<std::mutex> lck(mutex);
    if(all){
        conv.notify_all();
    }else{
        conv.notify_one();
    }
  }
 private:
  std::mutex mutex;
  std::condition_variable conv;
};

class MutexLockHolder{
public:
    MutexLockHolder(MutexLock& lock):m_lock(lock){
        lock.lock();
    }
    ~MutexLockHolder(){
        m_lock.unlock();
    }

private:
    MutexLock& m_lock;
};

class CppLockHolder{
public:
    CppLockHolder(CppLock& lock):m_lock(lock){
        lock.lock();
    }
    ~CppLockHolder(){
        m_lock.unlock();
    }

private:
    CppLock& m_lock;
};

}
