#ifndef m_LOCKER_H
#define m_LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>
#include "spdlog/spdlog.h"

//RAII封装 锁、信号量 多线程同步，确保任一时刻只能有一个线程能进入关键代码段
//信号量
class sem {
public:
    sem() {
        //sem_init() 成功时返回 0；错误时，返回 -1
        if (sem_init(&m_sem, 0, 0) != 0) {
            spdlog::error("sem_init error");
        }
    }
    sem(int num) {
        //num : 指定信号量值的大小
        if (sem_init(&m_sem, 0, num) != 0) {
            spdlog::error("sem_init error");
        }
    }
    ~sem() {
        sem_destroy(&m_sem);
    }
    bool wait() {
        //以原子操作方式将信号量减一,信号量为0时,sem_wait阻塞
        return sem_wait(&m_sem) == 0;
    }
    bool post() {
        //以原子操作方式将信号量加一,信号量大于0时,唤醒调用sem_post的线程
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};

//互斥锁
class locker {
public:
    locker() {
        //成功返回0，失败返回errno
        if (pthread_mutex_init(&m_mutex, NULL) != 0) {
            spdlog::error("pthread_mutex_init error");
        }
    }
    ~locker() {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock() {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock() {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    pthread_mutex_t *get() {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};

#endif
