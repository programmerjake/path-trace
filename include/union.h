#ifndef UNION_H
#define UNION_H

#include "object.h"

namespace PathTrace
{

class Union : public Object
{
public:
    Union(Object * a, Object * b);
    virtual ~Union();
    virtual SpanIterator * makeSpanIterator() const;
    virtual Object *duplicate() const
    {
        return new Union(a->duplicate(), b->duplicate());
    }
    virtual Object *transform(const Matrix &m) const
    {
        return new Union(PathTrace::transform(m, a), PathTrace::transform(m, a));
    }
private:
    Object * const a;
    Object * const b;
};

}

#endif // UNION_H
