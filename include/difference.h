#ifndef DIFFERENCE_H
#define DIFFERENCE_H

#include "object.h"

namespace PathTrace
{

class Difference : public Object
{
public:
    Difference(Object * a, Object * b);
    virtual ~Difference();
    virtual SpanIterator * makeSpanIterator() const;
    virtual Object *duplicate() const
    {
        return new Difference(a->duplicate(), b->duplicate());
    }
    virtual Object *transform(const Matrix &m) const
    {
        return new Difference(PathTrace::transform(m, a), PathTrace::transform(m, a));
    }
protected:
private:
    Object * const a;
    Object * const b;
};

}

#endif // DIFFERENCE_H
