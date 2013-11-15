#ifndef PLANE_H
#define PLANE_H

#include "object.h"

namespace PathTrace
{

class Plane : public Object
{
public:
    Plane(Vector3D normal, float d, const Material * material);
    Plane(Vector3D normal, Vector3D pos, const Material * material);
    virtual SpanIterator * makeSpanIterator() const;
    virtual ~Plane();
protected:
private:
    const Vector3D normal;
    const float d;
    const Material * const material;
};

}

#endif // PLANE_H
