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
    public SphereSpanIterator(const Sphere & sphere_in, const Ray & ray)
        : sphere(sphere_in)
    {
        ended = false;
        Vector3D origin_minus_center = ray.origin - sphere.center;
        double b = dot(origin_minus_center, ray.dir);
        double c = dot(origin_minus_center, origin_minus_center) - sphere.r_squared;
        double sqrt_arg = b * b - c;
        if(sqrt_arg <= eps)
        {
            ended = true;
            return;
        }
        double sqrt_v = std::sqrt(sqrt_arg);
        theSpan.start = -b - sqrt_v;
        theSpan.end = -b + sqrt_v;
        theSpan.startMaterial = sphere.material;
        theSpan.endMaterial = sphere.material;
        theSpan.startNormal = normalize(ray.getPoint(theSpan.start) - sphere.center);
        theSpan.endNormal = normalize(ray.getPoint(theSpan.end) - sphere.center);
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
    const Sphere & sphere;
    private bool ended;
};
}

SpanIterator * makeSpanIterator(const Ray & ray)
{
    return new SphereSpanIterator(*this, ray);
}

}
