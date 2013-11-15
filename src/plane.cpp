#include "plane.h"

namespace PathTrace
{

Plane::Plane(Vector3D normal, float d, const Material * material)
    : normal(normal), d(d), material(material)
{
}

Plane::Plane(Vector3D normal, Vector3D pos, const Material * material)
    : normal(normal), d(-dot(normal, pos)), material(material)
{
}

Plane::~Plane()
{
}

namespace
{

class PlaneSpanIterator : public SpanIterator
{
public:
    PlaneSpanIterator(const Vector3D normal, const float d, const Material * material)
        : normal(normal), d(d)
    {
        theSpan.startNormal = normalize(normal);
        theSpan.endNormal = normalize(normal);
        theSpan.startMaterial = material;
        theSpan.endMaterial = material;
        ended = true;
    }
    virtual void init(const Ray & ray)
    {
        ended = false;
        float divisor = dot(ray.dir, normal);
        float numerator = -d - dot(ray.origin, normal);
        float t;
        if(std::abs(divisor) < eps * eps || std::abs(t = numerator / divisor) >= max_value)
        {
            if(std::abs(numerator) < eps * eps)
            {
                theSpan.start = -max_value;
                theSpan.end = max_value;
            }
            else
            {
                ended = true;
            }
        }
        else if(divisor < 0)
        {
            theSpan.start = t;
            theSpan.end = max_value;
        }
        else
        {
            theSpan.start = -max_value;
            theSpan.end = t;
        }
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
    virtual ~PlaneSpanIterator()
    {
    }

private:
    Span theSpan;
    bool ended;
    const Vector3D normal;
    const float d;
};

}

SpanIterator * Plane::makeSpanIterator() const
{
    return new PlaneSpanIterator(normal, d, material);
}

}
