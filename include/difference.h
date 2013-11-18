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
protected:
private:
    Object * const a;
    Object * const b;
};

}

#endif // DIFFERENCE_H
