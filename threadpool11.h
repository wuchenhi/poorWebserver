#ifndef M_THREADPOOL11_H
#define M_THREADPOOL11_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include "spdlog/spdlog.h"

//使用一个工作队列完全解除了主线程和工作线程的耦合关系：主线程往工作队列中插入任务，工作线程通过竞争来取得任务并执行它。
//半同步/半反应堆
//template <typename T>
class threadpool {
public:
    //gdb调试时，先将线程池数量减小为1，观察逻辑是否正确，然后增加线程数，观察同步是否正确
    //单变量+explicit 防止隐式转换  thread_number是线程池中线程的数量
    threadpool() = default;
    explicit threadpool( int threadNumber = 8) : PoolPtr(std::make_shared<Pool>()) {
        for (int i = 0; i < threadNumber; ++i) {
            std::thread([pool = PoolPtr] {
                std::unique_lock<std::mutex> locker(pool->PoolMutex);
                while(true) {
                    if(!pool->Tasks.empty()) {
                        auto task = std::move(pool->Tasks.front());
                        pool->Tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    } else if(pool->isClosed) break;
                    else pool->PoolCond.wait(locker);
                }
            }).detach();      
        }
    }

    threadpool(threadpool&& ) = default;

    ~threadpool() {
        if(static_cast<bool>(PoolPtr)) {
            std::lock_guard<std::mutex> locker(PoolPtr->PoolMutex);
            PoolPtr->isClosed = true;
        }
        PoolPtr->PoolCond.notify_all(); 
    }

    // 通过queue容器创建请求队列，向队列中添加时，通过lock_guard保证线程安全，添加完成后通过notify_one提醒有任务要处理
    template<class F>
    void appendTask(F&& task) {
        // lock_guard 在代码块中有效
        {
            std::lock_guard<std::mutex> guard(PoolPtr->PoolMutex);
            PoolPtr->Tasks.emplace(std::forward<F>(task));
        }
        PoolPtr->PoolCond.notify_one(); 
    }

private:
    struct Pool {
        std::mutex PoolMutex;
        std::condition_variable PoolCond;
        std::queue<std::function<void()>> Tasks;
        bool isClosed;
    };
    // 用智能指针来管理 Pool
    std::shared_ptr<Pool> PoolPtr;
};

#endif
