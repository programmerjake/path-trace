#ifndef PATH_TRACE_H_INCLUDED
#define PATH_TRACE_H_INCLUDED

#include "misc.h"
#include "vector3d.h"
#include "span.h"
#include "ray.h"
#include "material.h"
#include "object.h"
#include "sphere.h"
#include "union.h"
#include "plane.h"
#include "intersection.h"
#include "difference.h"
#include <random>
#include <cstdint>

namespace PathTrace
{

class DefaultRandomEngine
{
public:
    DefaultRandomEngine()
    {
        seed(0);
    }
    static unsigned min()
    {
        return 0;
    }
    static unsigned max()
    {
        return 0xFFFFFFFF;
    }
    void seed(unsigned newValue)
    {
        v = newValue ^ 0x12476242;
    }
    unsigned operator ()()
    {
        v = (214013 * v + 2531011);
        return (unsigned)(v >> 32);
    }
    void discard(unsigned long long count)
    {
        for(unsigned long long i=0; i<count;i++)
            operator()();
    }
private:
    std::uint64_t v;
};

extern DefaultRandomEngine defaultRandomEngine;
const int DefaultRayDepth = 16;
template <typename T = DefaultRandomEngine>
inline Color traceRay(const Ray & ray, SpanIterator & spanIterator, int depth = DefaultRayDepth, T & randomEngine = defaultRandomEngine, float strength = 1.0)
{
    spanIterator.init(ray);
    float t = -1;
    const Material * material;
    Vector3D normal;
    float ior = 1;
    for(; spanIterator; spanIterator++)
    {
        if(spanIterator->start >= max_value)
        {
            return Color(0, 0, 0);
        }
        if(spanIterator->start >= eps)
        {
            t = spanIterator->start;
            normal = spanIterator->startNormal;
            material = spanIterator->startMaterial;
            assert(material != nullptr);
            assert(material->ior > eps);
            ior = 1.0 / material->ior;
            break;
        }
        if(spanIterator->end >= max_value)
        {
            return Color(0, 0, 0);
        }
        if(spanIterator->end >= eps)
        {
            t = spanIterator->end;
            normal = -spanIterator->endNormal;
            material = spanIterator->endMaterial;
            assert(material != nullptr);
            assert(material->ior > eps);
            ior = material->ior;
            break;
        }
    }
    if(t == -1)
    {
        return Color(0, 0, 0);
    }
    //ior = 1 / ior;
    Color retval = material->emissive;
    if(depth <= 0 || strength < 0.02)
    {
        return retval;
    }
    std::uniform_real_distribution<float> zeroToOne(0, 1);
    if(zeroToOne(randomEngine) < material->transmit_reflect_coefficient * ray.dir.refractStrength(ior, normal)) // transmit
    {
        Vector3D refractedRayDir = ray.dir.refract(ior, normal);
        if(refractedRayDir != Vector3D(0, 0, 0))
        {
            Ray newRay = Ray(ray.getPoint(t), refractedRayDir);
            retval += material->transmit * traceRay(newRay, spanIterator, depth - 1, randomEngine, strength * abs(material->transmit));
            return retval;
        }
    }

    // diffuse/specular reflect

    Vector3D reflectedRayDir = ray.dir.reflect(normal);
    Vector3D resultingRayDir = reflectedRayDir;
    assert(material->scatter_coefficient >= 0 && material->scatter_coefficient <= 1);
    if(material->scatter_coefficient > eps)
    {
        int count = 0;
        do
        {
            count++;
            assert(count <= 1000);
            if(count > 1000)
            {
                return retval;
            }
            resultingRayDir = Vector3D::rand(randomEngine, 1, 0);
            resultingRayDir += (1 / material->scatter_coefficient - 1) * reflectedRayDir;
        }
        while(dot(normal, resultingRayDir) <= eps);
        resultingRayDir = normalize(resultingRayDir);
    }

    float factor = 1 - (1 - dot(resultingRayDir, normal)) * material->scatter_coefficient;
    Ray newRay = Ray(ray.getPoint(t), resultingRayDir);
    return retval + factor * material->reflect * traceRay(newRay, spanIterator, depth - 1, randomEngine, strength * factor * abs(material->reflect));
}

const int DefaultSampleCount = 200;
const float DefaultScreenWidth = 4.0 / 3.0;
const float DefaultScreenHeight = 1.0;
const float DefaultScreenDistance = 2.0;

template <typename T = DefaultRandomEngine>
inline Color tracePixel(SpanIterator & spanIterator, float px, float py, float screenXResolution, float screenYResolution, int sampleCount = DefaultSampleCount, int rayDepth = DefaultRayDepth, float screenWidth = DefaultScreenWidth, float screenHeight = DefaultScreenHeight, float screenDistance = DefaultScreenDistance, T & randomEngine = defaultRandomEngine)
{
    float x = 2 * px / screenXResolution - 1;
    float y = 1 - 2 * py / screenYResolution;
    Ray ray = Ray(Vector3D(0, 0, 0), Vector3D(x * screenWidth, y * screenHeight, -screenDistance));
    Color retval = Color(0, 0, 0);
    for(int i = 0; i < sampleCount; i++)
    {
        retval += traceRay(ray, spanIterator, rayDepth, randomEngine);
    }
    retval /= sampleCount;
    return retval;
}

template <typename T = DefaultRandomEngine>
inline Color tracePixel(SpanIterator & spanIterator, int px, int py, int screenXResolution, int screenYResolution, int sampleCount = DefaultSampleCount, int rayDepth = DefaultRayDepth, float screenWidth = DefaultScreenWidth, float screenHeight = DefaultScreenHeight, float screenDistance = DefaultScreenDistance, T & randomEngine = defaultRandomEngine)
{
    std::uniform_real_distribution<float> zeroToOne(0, 1);
    Color retval = Color(0, 0, 0);
    for(int i = 0; i < sampleCount; i++)
    {
        float x = 2 * (px + zeroToOne(randomEngine)) / screenXResolution - 1;
        float y = 1 - 2 * (py + zeroToOne(randomEngine)) / screenYResolution;
        Ray ray = Ray(Vector3D(0, 0, 0), Vector3D(x * screenWidth, y * screenHeight, -screenDistance));
        retval += traceRay(ray, spanIterator, rayDepth, randomEngine);
    }
    retval /= sampleCount;
    return retval;
}
}

#endif // PATH-TRACE_H_INCLUDED
