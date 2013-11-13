#ifndef OBJECT_H
#define OBJECT_H

#include "span.h"
#include "ray.h"

namespace PathTrace
{

class Object
{
public:
    Object();
    virtual SpanIterator * makeSpanIterator(const Ray & ray) = 0;
protected:
private:
};

}

#endif // OBJECT_H
