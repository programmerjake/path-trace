#ifndef MUTEX_H_INCLUDED
#define MUTEX_H_INCLUDED

#include <pthread.h>

class mutex
{
private:
    pthread_mutex_t m;
public:
    mutex()
    {
        pthread_mutex_init(&m, NULL);
    }
    ~mutex()
    {
        pthread_mutex_destroy(&m);
    }
    void lock()
    {
        pthread_mutex_lock(&m);
    }
    void unlock()
    {
        pthread_mutex_unlock(&m);
    }
    friend class condition_variable;
};

#endif // MUTEX_H_INCLUDED
