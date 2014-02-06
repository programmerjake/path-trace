#ifndef SPAN_H
#define SPAN_H

#include "vector3d.h"
#include "material.h"
#include "ray.h"

namespace PathTrace
{

class Span
{
public:
    float start;
    Vector3D startNormal;
    const Material * startMaterial;
    float end;
    Vector3D endNormal;
    const Material * endMaterial;

    Span()
    {
        start = 0;
        startNormal = Vector3D(0, 0, 0);
        startMaterial = NULL;
        end = 0;
        endNormal = Vector3D(0, 0, 0);
        endMaterial = NULL;
    }

    Span(float start, Vector3D startNormal, const Material * startMaterial, float end, Vector3D endNormal, const Material * endMaterial)
    {
        this->start = start;
        this->startNormal = startNormal;
        this->startMaterial = startMaterial;
        this->end = end;
        this->endNormal = endNormal;
        this->endMaterial = endMaterial;
    }

    virtual ~Span()
    {
    }

    Span(const Span & rt)
    {
        start = rt.start;
        startNormal = rt.startNormal;
        startMaterial = rt.startMaterial;
        end = rt.end;
        endNormal = rt.endNormal;
        endMaterial = rt.endMaterial;
    }

    const Span & operator =(const Span & rt)
    {
        start = rt.start;
        startNormal = rt.startNormal;
        startMaterial = rt.startMaterial;
        end = rt.end;
        endNormal = rt.endNormal;
        endMaterial = rt.endMaterial;
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

    void copyStartFromStart(const Span & span)
    {
        start = span.start;
        startMaterial = span.startMaterial;
        startNormal = span.startNormal;
    }

    void copyEndFromStart(const Span & span)
    {
        end = span.start;
        endMaterial = span.startMaterial;
        endNormal = -span.startNormal;
    }

    void copyStartFromEnd(const Span & span)
    {
        start = span.end;
        startMaterial = span.endMaterial;
        startNormal = -span.endNormal;
    }

    void copyEndFromEnd(const Span & span)
    {
        end = span.end;
        endMaterial = span.endMaterial;
        endNormal = span.endNormal;
    }
};

class SpanIterator
{
public:
    virtual const Span & operator *() const = 0;
    virtual const Span * operator ->() const = 0;
    virtual bool isAtEnd() const = 0;
    virtual void next() = 0;
    virtual void init(const Ray & ray) = 0;

    virtual ~SpanIterator()
    {
    }

    bool operator !() const
    {
        return isAtEnd();
    }

    operator bool() const
    {
        return !isAtEnd();
    }

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
