#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#include <pthread.h>
#include <unistd.h>

class thread
{
private:
    pthread_t th;
    struct argStruct
    {
        virtual void run() = 0;
        virtual ~argStruct() {}
    };
    struct arg0Struct : public argStruct
    {
        void (*fn)();
        arg0Struct(void (*fn)())
            : fn(fn)
        {
        }
        virtual void run()
        {
            fn();
        }
    };
    template<typename T>
    struct arg1Struct : public argStruct
    {
        void (*fn)(T);
        T arg;
        arg1Struct(void (*fn)(T), T arg)
            : fn(fn), arg(arg)
        {
        }
        virtual void run()
        {
            fn(arg);
        }
    };
    thread(const thread &);
    void operator =(const thread &);
    static void * run(void * arg)
    {
        ((argStruct *)arg)->run();
        delete (argStruct *)arg;
        return NULL;
    }
public:
    thread()
        : th(0)
    {
    }
    thread(void (*fn)())
    {
        pthread_create(&th, NULL, &run, (void *)new arg0Struct(fn));
    }
    template<typename T>
    thread(void (*fn)(T arg), T fnArg)
    {
        pthread_create(&th, NULL, &run, (void *)new arg1Struct<T>(fn, fnArg));
    }
    void detach()
    {
        pthread_detach(th);
    }
    void join()
    {
        pthread_join(th, NULL);
    }
    static int hardware_concurrency()
    {
        return sysconf(_SC_NPROCESSORS_ONLN);
    }
};

namespace chrono
{
    typedef int seconds;
}

namespace this_thread
{
    inline void sleep_for(chrono::seconds seconds)
    {
        sleep(seconds);
    }
};

#endif // THREAD_H_INCLUDED
