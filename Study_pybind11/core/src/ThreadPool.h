#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

/**
demo:
// create thread pool with 4 worker threads
ThreadPool pool(4);

// enqueue and store future
auto result = pool.enqueue([](int answer) { return answer; }, 42);

// get result from future
std::cout << result.get() << std::endl;
*/
namespace h7 {

class ThreadPool{
public:
    template<typename T>
    using SPT = std::shared_ptr<T>;

    ThreadPool(size_t);
    ~ThreadPool();

    bool isStoped();
    void stop();

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

    template<class F, class... Args>
    static auto batchRun(int tc, int start, int end, F&& f, Args&&... args)
        -> SPT<std::vector<typename std::result_of<F(int, Args...)>::type>>;

    //exclude: end.
    //include: start
    static inline int batchRawRun(int tc, int start, int end,
                            std::function<bool(int)> func);

private:
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;
    // the task queue
    std::queue< std::function<void()> > tasks;
    
    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    volatile bool stop_;
};

inline bool ThreadPool::isStoped(){
     std::unique_lock<std::mutex> lock(queue_mutex);
     return stop_;
}
inline void ThreadPool::stop(){
    std::unique_lock<std::mutex> lock(queue_mutex);
    stop_ = true;
}
 
// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads)
    : stop_(false)
{
    for(size_t i = 0;i<threads;++i)
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->stop_ || !this->tasks.empty(); });
                        if(this->stop_ && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            }
        );
}

// add new work item to the pool
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if(stop_){
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

template<class F, class... Args>
auto ThreadPool::batchRun(int tc, int start, int end, F&& f, Args&&... args)
    -> SPT<std::vector<typename std::result_of<F(int, Args...)>::type>>{
    using EleType = typename std::result_of<F(int, Args...)>::type;
    const int c = end - start;
    auto rets = std::make_shared<std::vector<EleType>>();
    rets->resize(c);
    {
        tc = c < tc ? c : tc;
        ThreadPool tp(tc);
        //
        const int every = c / tc + ((c % tc != 0) ? 1 : 0);
        //
        for(int i = start; i < end ; i += every){
            auto task = std::make_shared<std::packaged_task<EleType(int)> >(
                    std::bind(std::forward<F>(f),
                              std::placeholders::_1,
                              std::forward<Args>(args)...)
                );
            const int ki = i;
            const int kend = (i + every) < end ? (i + every) : end;
            tp.enqueue([rets, task, ki, kend](){
                for(int k = ki ; k < kend ; ++k){
                    std::future<EleType> res = task->get_future();
                    (*task)(k);
                    (*rets)[k] = std::move(res.get());
                    task->reset();
                }
            });
        }
    }
    return rets;
}

int ThreadPool::batchRawRun(int tc, int start, int end,
                        std::function<bool(int)> func){
    const int c = end - start;
    tc = c < tc ? c : tc;
    std::vector<int> states;
    states.resize(c);
    using PKT = std::packaged_task<bool(int)>;
    //using SPPKT = std::shared_ptr<PKT>;
    {
        ThreadPool tp(tc);
        //
        // c = 17, tc = 12, every = 1, left = 5
        const int every = c / tc;
        const int left = c - tc * every;
        //
        for(int i = 0, si = start; i < tc ; ++i){
             const int ki = si;
             const int act_count = i < left ? every + 1 : every;
             si += act_count;
             auto task = std::make_shared<PKT>(
                         std::bind(func, std::placeholders::_1)
                     );
             tp.enqueue([task, ki, start, act_count, &states](){
                 for(int k = ki, kend = ki + act_count; k < kend ; ++k){
                     auto fut = task->get_future();
                     (*task)(k);
                     bool ret = fut.get();
                     task->reset();
                     //printf(" k = %d, states_i = %d\n", k, k - start);
                     states[k - start] = ret ? 1 : 0;
                     if(!ret){
                         break;
                     }
                 }
             });
        }
    }
    for(auto& s : states){
        if(s == 0){
            return 0;
        }
    }
    //all success
    return tc;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop_ = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}
}
