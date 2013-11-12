#ifndef OBJECT_H
#define OBJECT_H

#include "path-trace.h"
#include "span.h"

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
