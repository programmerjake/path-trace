#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <cmath>
#include <random>
#include <assert.h>

namespace PathTrace
{

class Vector3D
{
public:
    double x, y, z;
    Vector3D()
    {
        x = 0;
        y = 0;
        z = 0;
    }
    Vector3D(double x, double y, double z)
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
    friend Vector3D operator /(const Vector3D & l, double r)
    {
        return Vector3D(l.x / r, l.y / r, l.z / r);
    }
    friend Vector3D operator *(const Vector3D & l, double r)
    {
        return Vector3D(l.x * r, l.y * r, l.z * r);
    }
    friend Vector3D operator *(double l, const Vector3D & r)
    {
        return operator *(r, l);
    }
    friend double dot(const Vector3D & l, const Vector3D & r)
    {
        Vector3D v = l * r;
        return v.x + v.y + v.z;
    }
    friend double abs_squared(const Vector3D & v)
    {
        return dot(v, v);
    }
    friend double abs(const Vector3D & v)
    {
        return std::sqrt(abs_squared(v));
    }
    friend Vector3D normalize(const Vector3D & v)
    {
        double magnitude = abs(v);
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
    const Vector3D & operator +=(const Vector & r)
    {
        *this = *this + r;
        return *this;
    }
    const Vector3D & operator -=(const Vector & r)
    {
        *this = *this - r;
        return *this;
    }
    const Vector3D & operator *=(const Vector & r)
    {
        *this = *this * r;
        return *this;
    }
    const Vector3D & operator /=(const Vector & r)
    {
        *this = *this / r;
        return *this;
    }
    const Vector3D & operator *=(double r)
    {
        *this = *this * r;
        return *this;
    }
    const Vector3D & operator /=(double r)
    {
        *this = *this / r;
        return *this;
    }
    template<typename randomNumberGeneratorType>
    static Vector3D rand(randomNumberGeneratorType & r, double max = 1, double min = 0)
    {
        assert(max >= 0);
        assert(min < max || min == 0);
        if(max == 0)
            return Vector3D(0, 0, 0);
        std::uniform_real_distribution<double> distribution(-max,max);
        Vector3D retval;
        double mag;
        do
        {
            retval.x = distribution(r);
            retval.y = distribution(r);
            retval.z = distribution(r);
            mag = abs(retval);
        }
        while(mag > max || (min > 0 && mag == 0));
        if(min > 0)
            return retval * ((max + mag * (max - min)) / mag);
        return retval;
    }
protected:
private:
};

}

#endif // VECTOR3D_H
