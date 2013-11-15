#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <cmath>
#include <random>
#include <assert.h>
#include <iostream>
#include "misc.h"

namespace PathTrace
{

class Vector3D
{
public:
    float x, y, z;
    Vector3D()
    {
        x = 0;
        y = 0;
        z = 0;
    }
    Vector3D(float x, float y, float z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }
    ~Vector3D()
    {
    }
    Vector3D(const Vector3D & rt)
    {
        x = rt.x;
        y = rt.y;
        z = rt.z;
    }
    const Vector3D & operator =(const Vector3D & rt)
    {
        x = rt.x;
        y = rt.y;
        z = rt.z;
        return *this;
    }
    friend Vector3D operator +(const Vector3D & l, const Vector3D & r)
    {
        return Vector3D(l.x + r.x, l.y + r.y, l.z + r.z);
    }
    friend Vector3D operator -(const Vector3D & l, const Vector3D & r)
    {
        return Vector3D(l.x - r.x, l.y - r.y, l.z - r.z);
    }
    friend Vector3D operator *(const Vector3D & l, const Vector3D & r)
    {
        return Vector3D(l.x * r.x, l.y * r.y, l.z * r.z);
    }
    friend Vector3D operator /(const Vector3D & l, const Vector3D & r)
    {
        return Vector3D(l.x / r.x, l.y / r.y, l.z / r.z);
    }
    friend Vector3D operator /(const Vector3D & l, float r)
    {
        return Vector3D(l.x / r, l.y / r, l.z / r);
    }
    friend Vector3D operator *(const Vector3D & l, float r)
    {
        return Vector3D(l.x * r, l.y * r, l.z * r);
    }
    friend Vector3D operator *(float l, const Vector3D & r)
    {
        return operator *(r, l);
    }
    friend float dot(const Vector3D & l, const Vector3D & r)
    {
        Vector3D v = l * r;
        return v.x + v.y + v.z;
    }
    friend float abs_squared(const Vector3D & v)
    {
        return dot(v, v);
    }
    friend float abs(const Vector3D & v)
    {
        return std::sqrt(abs_squared(v));
    }
    friend Vector3D normalize(const Vector3D & v)
    {
        float magnitude = abs(v);
        if(magnitude == 0) magnitude = 1;
        return v / magnitude;
    }
    Vector3D operator -() const
    {
        return Vector3D(-x, -y, -z);
    }
    friend bool operator ==(const Vector3D & l, const Vector3D & r)
    {
        return l.x == r.x && l.y == r.y && l.z == r.z;
    }
    friend bool operator !=(const Vector3D & l, const Vector3D & r)
    {
        return l.x != r.x || l.y != r.y || l.z != r.z;
    }
    const Vector3D & operator +=(const Vector3D & r)
    {
        *this = *this + r;
        return *this;
    }
    const Vector3D & operator -=(const Vector3D & r)
    {
        *this = *this - r;
        return *this;
    }
    const Vector3D & operator *=(const Vector3D & r)
    {
        *this = *this * r;
        return *this;
    }
    const Vector3D & operator /=(const Vector3D & r)
    {
        *this = *this / r;
        return *this;
    }
    const Vector3D & operator *=(float r)
    {
        *this = *this * r;
        return *this;
    }
    const Vector3D & operator /=(float r)
    {
        *this = *this / r;
        return *this;
    }
    template<typename randomNumberGeneratorType>
    static Vector3D rand(randomNumberGeneratorType & r, float max = 1, float min = 0)
    {
        assert(max >= 0);
        assert(min <= max || min == 0);
        if(max == 0)
            return Vector3D(0, 0, 0);
        std::uniform_real_distribution<float> distribution(-max,max);
        Vector3D retval;
        float mag;
        do
        {
            retval.x = distribution(r);
            retval.y = distribution(r);
            retval.z = distribution(r);
            mag = abs(retval);
        }
        while(mag > max || (min > 0 && mag == 0));
        if(min > 0)
            retval *= ((max + mag * (max - min)) / mag);
        //std::cout << retval << std::endl;
        return retval;
    }
    Vector3D reflect(Vector3D normal) const
    {
        normal = normalize(normal);
        return *this - 2 * dot(*this, normal) * normal;
    }
    float refractStrength(float relative_ior, Vector3D normal) const
    {
        if(relative_ior < eps || relative_ior > 1 / eps || normal == Vector3D(0, 0, 0) || *this == Vector3D(0, 0, 0))
            return 0;
        normal = normalize(normal);
        Vector3D incident = normalize(*this);
        float incident_dot_normal = dot(incident, normal);
        return 1 - relative_ior * relative_ior * (1 - incident_dot_normal * incident_dot_normal);
    }
    Vector3D refract(float relative_ior, Vector3D normal) const
    {
        if(relative_ior < eps || relative_ior > 1 / eps || normal == Vector3D(0, 0, 0) || *this == Vector3D(0, 0, 0))
            return Vector3D(0, 0, 0);
        normal = normalize(normal);
        Vector3D incident = normalize(*this);
        float incident_dot_normal = dot(incident, normal);
        float sqrt_arg = 1 - relative_ior * relative_ior * (1 - incident_dot_normal * incident_dot_normal);
        if(sqrt_arg < 0)
            return Vector3D(0, 0, 0);
        return normalize(relative_ior * incident - (relative_ior * incident_dot_normal + std::sqrt(sqrt_arg)) * normal);
    }
    friend std::ostream & operator <<(std::ostream & o, const Vector3D & v)
    {
        return o << "<" << v.x << ", " << v.y << ", " << v.z << ">";
    }
protected:
private:
};

}

#endif // VECTOR3D_H
