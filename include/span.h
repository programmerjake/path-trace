#ifndef SPAN_H
#define SPAN_H

#include "path-trace.h"

namespace PathTrace
{

class Span
{
public:
    double start;
    double end;

    Span()
    {
        start = 0;
        end = 0;
    }

    Span(double start, double end)
    {
        this->start = start;
        this->end = end;
    }

    virtual ~Span()
    {
    }

    Span(const Span & rt)
    {
        start = rt.start;
        end = rt.end;
    }

    const Span & operator =(const Span & rt)
    {
        start = rt.start;
        end = rt.end;
        return *this;
    }

    bool isEmpty() const
    {
        return end <= start;
    }

    operator bool() const
    {
        return end > start;
    }

    bool operator !() const
    {
        return end <= start;
    }

    friend bool overlaps(const Span & l, const Span & r)
    {
        if(!l || !r)
            return false;
        if(l.start > r.end)
            return false;
        if(l.end < r.start)
            return false;
        return true;
    }
};

class SpanIterator
{
public:
    virtual void free() = 0;
    virtual const Span & operator *() const = 0;
    virtual const Span * operator ->() const = 0;
    const Span * operator ->*() const
    {
        return operator ->();
    }
    virtual ~SpanIterator()
    {
    }

    virtual bool isAtEnd() const = 0;

    bool operator !() const
    {
        return isAtEnd();
    }

    operator bool() const
    {
        return !isAtEnd();
    }

    virtual void next() = 0;

    void operator ++(int)
    {
        next();
    }

    const SpanIterator & operator ++()
    {
        next();
        return *this;
    }


protected:
    SpanIterator()
    {
    }
};

}

#endif // SPAN_H
