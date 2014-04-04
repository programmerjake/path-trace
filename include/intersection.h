#ifndef INTERSECTION_H
#define INTERSECTION_H

#include "object.h"

namespace PathTrace
{

class Intersection : public Object
{
public:
    Intersection(Object * a, Object * b);
    virtual ~Intersection();
    virtual SpanIterator * makeSpanIterator() const;
    virtual Object *duplicate() const
    {
        return new Intersection(a->duplicate(), b->duplicate());
    }
    virtual Object *transform(const Matrix &m) const
    {
        return new Intersection(PathTrace::transform(m, a), PathTrace::transform(m, a));
    }
protected:
private:
    Object * const a;
    Object * const b;
};

}

#endif // INTERSECTION_H
