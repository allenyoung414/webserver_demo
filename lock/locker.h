#ifndef cond_H
#define cond_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class sem{
    private:
        sem_t m_sem;
    
    public:
    sem()
    {
        if(sem_init(&m_sem, 0, 0) != 0)
        {
            throw std::exception();
        }
    }
    sem(int num)
    {
        if(sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    ~sem()
    {
        sem_destroy(&m_sem);
    }
    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }
    bool post()
    {
        return sem_post(&m_sem) == 0;
    }
};
class cond
{
private:
    /* data */
    pthread_mutex_t m_mutex;
public:
    cond()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }
    }
    ~cond()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    pthread_mutex_t *get()
    {
        return &m_mutex;
    }
};
class cond
{
private:
    /* data */
    pthread_cond_t m_cond;
public:
    cond(/* args */);
    ~cond();
    bool wait(pthread_mutex_t *m_mutex)
    {
        int ret = 0;
        ret = pthread_cond_wait(&m_cond, m_mutex);
        return ret == 0;
    }
};

cond::cond(/* args */)
{
    if (pthread_cond_init(&m_cond, NULL) != 0)
        {
            //pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
}

cond::~cond()
{
    pthread_cond_destroy(&m_cond);
}

#endif