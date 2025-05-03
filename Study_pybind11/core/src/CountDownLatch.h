#pragma once

#include <mutex>
#include <condition_variable>
#include <chrono>
#include "utils/h_atomic.h"

namespace h7 {
    class CountDownLatch{

    public:
        CountDownLatch(int count):m_leftCount(count){}

        void countDown(int c = 1){
            int old = h_atomic_add(&m_leftCount, -c);
            if(old == c){
                std::unique_lock<std::mutex> lck(m_mutex);
                m_reached = true;
                m_cv.notify_all();
            }
        }

        int getCount(){
            return h_atomic_get(&m_leftCount);
        }
        //true means count down reached. or false means timeout
        bool await(unsigned long long timeout = 0){
            if(timeout == 0){
                //already done
                if(getCount() == 0){
                    return true;
                }
                {
                    std::unique_lock<std::mutex> lck(m_mutex);
                    m_cv.wait(lck, [this](){
                       return m_reached;
                    });
                    return true;
                }
            }else{
                using TimeT = long long;
                using _Clock = std::chrono::system_clock;
                using _Msec = std::chrono::milliseconds;
                //
                std::unique_lock<std::mutex> lck(m_mutex);
                auto msec = std::chrono::duration_cast<_Msec>(
                            _Clock::now().time_since_epoch()).count();
                auto dst_msec = std::chrono::milliseconds(msec + (TimeT)timeout);
                auto tp1 = std::chrono::time_point<_Clock,_Msec>(dst_msec);
                // wait_until
                return m_cv.wait_until(lck, tp1, [this](){
                    return m_reached;
                 });
            }
        }

    private:
        std::mutex m_mutex;
        std::condition_variable m_cv;
        volatile int m_leftCount;
        bool m_reached {false};
    };
}
