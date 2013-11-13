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
#include <random>

namespace PathTrace
{
extern std::default_random_engine DefaultRandomEngine;
const int DefaultRayDepth = 16;
template <typename T = std::default_random_engine>
inline Color traceRay(const Ray & ray, const Object * world, int depth = DefaultRayDepth, T & randomEngine = DefaultRandomEngine)
{
    SpanIterator & i = *world->makeSpanIterator(ray);
    AutoDestruct<SpanIterator> autoDestruct1(&i);
    double t = -1;
    const Material * material;
    Vector3D normal;
    double ior = 1;
    for(; i; i++)
    {
        if(i->start >= max_value)
            return Color(0, 0, 0);
        if(i->start >= eps)
        {
            t = i->start;
            normal = i->startNormal;
            material = i->startMaterial;
            assert(material != nullptr);
            ior = material->ior;
            break;
        }
        if(i->end >= max_value)
            return Color(0, 0, 0);
        if(i->end >= eps)
        {
            t = i->end;
            normal = i->endNormal;
            material = i->endMaterial;
            assert(material != nullptr);
            assert(material->ior > eps);
            ior = 1.0 / material->ior;
            break;
        }
    }
    if(t == -1)
        return Color(0, 0, 0);
    Color retval = material->emissive;
    if(depth <= 0)
        return retval;
    std::uniform_real_distribution<double> zeroToOne(0, 1);
    if(zeroToOne(randomEngine) < material->transmit_reflect_coefficient) // transmit
    {
        Vector3D refractedRayDir = ray.dir.refract(ior, normal);
        if(refractedRayDir != Vector3D(0, 0, 0))
        {
            Ray newRay = Ray(ray.getPoint(t), refractedRayDir);
            retval += material->transmit * traceRay(newRay, world, depth - 1, randomEngine);
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
                return retval;
            resultingRayDir = Vector3D::rand(randomEngine, 1, 0);
            resultingRayDir += (1 / material->scatter_coefficient - 1) * reflectedRayDir;
        }
        while(dot(normal, resultingRayDir) <= eps);
        resultingRayDir = normalize(resultingRayDir);
    }

    double factor = 1 - (1 - dot(resultingRayDir, normal)) * material->scatter_coefficient;
    Ray newRay = Ray(ray.getPoint(t), resultingRayDir);
    return retval + factor * material->reflect * traceRay(newRay, world, depth - 1, randomEngine);
}

const int DefaultSampleCount = 200;
const double DefaultScreenWidth = 4.0 / 3.0;
const double DefaultScreenHeight = 1.0;
const double DefaultScreenDistance = 2.0;

template <typename T = std::default_random_engine>
inline Color tracePixel(const Object * world, double px, double py, double screenXResolution, double screenYResolution, int sampleCount = DefaultSampleCount, double screenWidth = DefaultScreenWidth, double screenHeight = DefaultScreenHeight, double screenDistance = DefaultScreenDistance, T & randomEngine = DefaultRandomEngine)
{
    double x = 2 * px / screenXResolution - 1;
    double y = 1 - 2 * py / screenYResolution;
    Ray ray = Ray(Vector3D(0, 0, 0), Vector3D(x * screenWidth, y * screenHeight, -screenDistance));
    Color retval = Color(0, 0, 0);
    for(int i = 0; i < sampleCount; i++)
    {
        retval += traceRay(ray, world, DefaultRayDepth, randomEngine);
    }
    retval /= sampleCount;
    return retval;
}

template <typename T = std::default_random_engine>
inline Color tracePixel(const Object * world, int px, int py, int screenXResolution, int screenYResolution, int sampleCount = DefaultSampleCount, double screenWidth = DefaultScreenWidth, double screenHeight = DefaultScreenHeight, double screenDistance = DefaultScreenDistance, T & randomEngine = DefaultRandomEngine)
{
    return tracePixel(world, px + 0.5, py + 0.5, (double)screenXResolution, (double)screenYResolution, sampleCount, screenWidth, screenHeight, screenDistance, randomEngine);
}
}

#endif // PATH-TRACE_H_INCLUDED
