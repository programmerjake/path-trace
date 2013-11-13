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
    virtual SpanIterator * makeSpanIterator(const Ray & ray) const;
protected:
private:
    Object * const a;
    Object * const b;
};

}

#endif // UNION_H
