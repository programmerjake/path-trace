#ifndef ATOMIC_H_INCLUDED
#define ATOMIC_H_INCLUDED

#include "mutex.h"

template <typename T>
class atomic_base
{
protected:
    mutable mutex m;
    T value;
public:
    atomic_base()
    {
    }
    atomic_base(T v)
        : value(v)
    {
    }
    const atomic_base<T> & operator =(const atomic_base<T> & rt)
    {
        rt.lock();
        T v = rt.value;
        rt.unlock();
        m.lock();
        value = v;
        m.unlock();
        return *this;
    }
    const atomic_base<T> & operator =(T v)
    {
        m.lock();
        value = v;
        m.unlock();
        return *this;
    }
    operator T()
    {
        m.lock();
        T retval = value;
        m.unlock();
        return retval;
    }
    T exchange(T newValue)
    {
        m.lock();
        T retval = value;
        value = newValue;
        m.unlock();
        return retval;
    }
};

class atomic_bool : public atomic_base<bool>
{
public:
    atomic_bool(bool v = false)
        : atomic_base<bool>(v)
    {
    }
    bool test_and_set(bool newValue)
    {
        return exchange(newValue);
    }
    const atomic_bool & operator =(bool v)
    {
        m.lock();
        value = v;
        m.unlock();
        return *this;
    }
};

class atomic_int : public atomic_base<int>
{
public:

    atomic_int(int v = 0)
        : atomic_base<int>(v)
    {
    }
    const atomic_int & operator =(int v)
    {
        m.lock();
        value = v;
        m.unlock();
        return *this;
    }
    const int operator ++()
    {
        m.lock();
        int retval = ++value;
        m.unlock();
        return retval;
    }
    const int operator ++(int)
    {
        m.lock();
        int retval = value++;
        m.unlock();
        return retval;
    }
    const int operator --()
    {
        m.lock();
        int retval = --value;
        m.unlock();
        return retval;
    }
    const int operator --(int)
    {
        m.lock();
        int retval = value--;
        m.unlock();
        return retval;
    }
};

class atomic_uint : public atomic_base<unsigned>
{
public:

    atomic_uint(unsigned v = 0)
        : atomic_base<unsigned>(v)
    {
    }
    const atomic_uint & operator =(unsigned v)
    {
        m.lock();
        value = v;
        m.unlock();
        return *this;
    }
    const unsigned operator ++()
    {
        m.lock();
        unsigned retval = ++value;
        m.unlock();
        return retval;
    }
    const unsigned operator ++(int)
    {
        m.lock();
        unsigned retval = value++;
        m.unlock();
        return retval;
    }
    const unsigned operator --()
    {
        m.lock();
        unsigned retval = --value;
        m.unlock();
        return retval;
    }
    const unsigned operator --(int)
    {
        m.lock();
        unsigned retval = value--;
        m.unlock();
        return retval;
    }
};

#endif // ATOMIC_H_INCLUDED
