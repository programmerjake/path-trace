#include "sphere.h"

namespace PathTrace
{

Sphere::Sphere(Vector3D center, float r, const Material * material)
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
    SphereSpanIterator(const Vector3D & center, float r_squared, const Material * material)
        : center(center), r_squared(r_squared)
    {
        theSpan.startMaterial = material;
        theSpan.endMaterial = material;
        ended = true;
    }
    virtual void init(const Ray & ray)
    {
        ended = false;
        Vector3D origin_minus_center = ray.origin - center;
        float b = dot(origin_minus_center, ray.dir);
        float c = dot(origin_minus_center, origin_minus_center) - r_squared;
        float sqrt_arg = b * b - c;
        if(sqrt_arg <= eps)
        {
            ended = true;
            return;
        }
        float sqrt_v = std::sqrt(sqrt_arg);
        theSpan.start = -b - sqrt_v;
        theSpan.end = -b + sqrt_v;
        theSpan.startNormal = normalize(ray.getPoint(theSpan.start) - center);
        theSpan.endNormal = normalize(ray.getPoint(theSpan.end) - center);
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
    const Vector3D center;
    const float r_squared;
};
}

SpanIterator * Sphere::makeSpanIterator() const
{
    return new SphereSpanIterator(center, r_squared, material);
}

}
