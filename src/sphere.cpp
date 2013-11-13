#include "sphere.h"

namespace PathTrace
{

Sphere::Sphere(Vector3D center, double r, const Material * material)
{
    this->center = center;
    this->r = r;
    this->r_squared = r * r;
    this->material = material;
}

Sphere::~Sphere()
{
}

namespace
{

class SphereSpanIterator : public SpanIterator
{
public:
    SphereSpanIterator(const Vector3D & center, double r_squared, const Material * material, const Ray & ray)
    {
        ended = false;
        Vector3D origin_minus_center = ray.origin - center;
        double b = dot(origin_minus_center, ray.dir);
        double c = dot(origin_minus_center, origin_minus_center) - r_squared;
        double sqrt_arg = b * b - c;
        if(sqrt_arg <= eps)
        {
            ended = true;
            return;
        }
        double sqrt_v = std::sqrt(sqrt_arg);
        theSpan.start = -b - sqrt_v;
        theSpan.end = -b + sqrt_v;
        theSpan.startMaterial = material;
        theSpan.endMaterial = material;
        theSpan.startNormal = normalize(ray.getPoint(theSpan.start) - center);
        theSpan.endNormal = normalize(ray.getPoint(theSpan.end) - center);
    }
    virtual void free()
    {
        delete this;
    }
    virtual const Span & operator *() const
    {
        return theSpan;
    }
    virtual const Span * operator ->() const
    {
        return &theSpan;
    }
    virtual bool isAtEnd() const
    {
        return ended;
    }
    virtual void next()
    {
        ended = true;
    }
    virtual ~SphereSpanIterator()
    {
    }

private:
    Span theSpan;
    bool ended;
};
}

SpanIterator * Sphere::makeSpanIterator(const Ray & ray)
{
    return new SphereSpanIterator(center, r_squared, material, ray);
}

}
