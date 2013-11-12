#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <cmath>

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
protected:
private:
};

}

#endif // VECTOR3D_H
