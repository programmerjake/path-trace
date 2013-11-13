#ifndef SPHERE_H
#define SPHERE_H

#include "object.h"

namespace PathTrace
{

class Sphere : public Object
{
public:
    Sphere(Vector3D center, double r, const Material * material);
    virtual ~Sphere();
    virtual SpanIterator * makeSpanIterator(const Ray & ray);
protected:
private:
    Vector3D center;
    const Material * material;
    double r, r_squared;
};

}

#endif // SPHERE_H
