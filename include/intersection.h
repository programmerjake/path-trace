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
protected:
private:
    Object * const a;
    Object * const b;
};

}

#endif // INTERSECTION_H
