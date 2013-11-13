#ifndef RAY_H_INCLUDED
#define RAY_H_INCLUDED

#include "vector3d.h"

namespace PathTrace
{

struct Ray
{
    Vector3D origin;
    Vector3D dir;
    Ray(Vector3D origin, Vector3D dir)
    {
        this->origin = origin;
        this->dir = normalize(dir);
        assert(this->dir != Vector3D(0, 0, 0));
    }
    Vector3D getPoint(double t) const
    {
        return origin + t * dir;
    }
};

}

#endif // RAY_H_INCLUDED
