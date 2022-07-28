#ifndef M_THREADPOOL_H
#define M_THREADPOOL_H

#include <deque>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "locker.h"
#include "CGImysql/sql_connection_pool.h"

#include "spdlog/spdlog.h"

//使用一个工作队列完全解除了主线程和工作线程的耦合关系：主线程往工作队列中插入任务，工作线程通过竞争来取得任务并执行它。
//半同步/半反应堆
template <typename T>
class threadpool {
public:
    ////gdb调试时，先将线程池数量减小为1，观察逻辑是否正确，然后增加线程数，观察同步是否正确
    //thread_number是线程池中线程的数量
    //max_requests是请求队列中最多允许的、等待处理的请求的数量
    threadpool(connection_pool *connPool, int thread_number = 8, int max_request = 10000);
    ~threadpool();
    bool append(T *request, int state);
    bool append_p(T *request);

private:
    //工作线程运行的函数，它不断从工作队列中取出任务并执行之，必须为静态函数，否则 this指针不能带入 pthread_creat的第三个参数
    static void *worker(void *arg);
    void run();

private:
    int m_thread_number;        //线程池中的线程数
    int m_max_requests;         //请求队列中允许的最大请求数
    pthread_t *m_threads;       //描述线程池的数组，其大小为m_thread_number
    std::deque<T *> m_workqueue; //deque请求队列 不用vector 因为在头部更改效率差
    locker m_queuelocker;       //保护请求队列的互斥锁
    sem m_queuestat;            //是否有任务需要处理
    connection_pool *m_connPool;  //数据库
};

//构造函数中创建线程池,pthread_create函数中将类的对象作为参数传递给静态函数(worker),在静态函数中引用>这个对象,并调用其动态方法(run)
//类对象传递时用this指针，传递给静态函数后，将其转换为线程池类，并调用私有成员函数run
template <typename T>
threadpool<T>::threadpool(connection_pool *connPool, int thread_number, int max_requests) : m_thread_number(thread_number), m_max_requests(max_requests), m_threads(NULL),m_connPool(connPool) {
    if (thread_number <= 0 || max_requests <= 0)
        throw std::exception();
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads) //分配内存失败
        throw std::exception();
    //循环创建线程，并将工作线程按要求进行运行
    for (int i = 0; i < thread_number; ++i) {
        //m_threads+i 变量地址名 ， NULL 默认属性的线程
        if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
            delete[] m_threads;
            throw std::exception();
        }
        //将线程进行分离后，不用单独对工作线程进行回收
        if (pthread_detach(m_threads[i])) {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template <typename T>
threadpool<T>::~threadpool() {
    delete[] m_threads;
}

//通过deque容器创建请求队列，向队列中添加时，通过互斥锁保证线程安全，添加完成后通过信号量提醒有任务要处理，最后注意线程同步。
template <typename T>
bool threadpool<T>::append(T *request, int state) {
    m_queuelocker.lock();
    //根据硬件，预先设置请求队列的最大值
    if (m_workqueue.size() >= m_max_requests) {
        m_queuelocker.unlock();
        return false;
    }
    request->m_state = state;
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();//append以后信号量 post      run-> work
    return true;
}

template <typename T>
bool threadpool<T>::append_p(T *request) {
    m_queuelocker.lock();
    if (m_workqueue.size() >= m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

template <typename T>
void *threadpool<T>::worker(void *arg) {
    //将参数强转为线程池类，调用成员方法
     threadpool *pool = static_cast<threadpool *>(arg);//this 指针导入
    pool->run();
    return pool;
}

template <typename T>
void threadpool<T>::run() {
    while (true) {
        //信号量等待
        m_queuestat.wait();
        //被唤醒后先加互斥锁
        m_queuelocker.lock();
        if (m_workqueue.empty()) {
            m_queuelocker.unlock();
            continue;
        }
        T *request = m_workqueue.front();//从请求队列中取出第一个任务
        m_workqueue.pop_front();//将任务从请求队列删除
        m_queuelocker.unlock();
        if (!request)
            continue;
        //reactor模式中，主线程(I/O处理单元)只负责监听文件描述符上是否有事件发生，
        //有的话立即通知工作线程(逻辑单元 )，读写数据、接受新连接及处理客户请求均在工作线程中完成。
        //通常由同步I/O实现。
        if (0 == request->m_state) {
            if (request->read_once()) {
                request->improv = 1;
                connectionRAII mysqlcon(&request->mysql, m_connPool);
                request->process();
            }
            else {
                request->improv = 1;
                request->timer_flag = 1;
            }
        }else {
            if (request->write()) {
                request->improv = 1;
            }else {
                request->improv = 1;
                request->timer_flag = 1;
            }
        }
    }
}

#endif
