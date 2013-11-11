#ifndef SPAN_H
#define SPAN_H

#include <iterator>

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
protected:
    SpanIterator()
    {
    }
};

}

#endif // SPAN_H
