#ifndef CONDITION_VARIABLE_H_INCLUDED
#define CONDITION_VARIABLE_H_INCLUDED

#include "mutex.h"

class condition_variable
{
private:
    pthread_cond_t c;
public:
    condition_variable()
    {
        pthread_cond_init(&c, NULL);
    }
    ~condition_variable()
    {
        pthread_cond_destroy(&c);
    }
    void notify_one()
    {
        pthread_cond_signal(&c);
    }
    void notify_all()
    {
        pthread_cond_broadcast(&c);
    }
    void wait(mutex & lock)
    {
        pthread_cond_wait(&c, &lock.m);
    }
};

typedef condition_variable condition_variable_any;

#endif // CONDITION_VARIABLE_H_INCLUDED
