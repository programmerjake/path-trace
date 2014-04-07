#ifndef TRANSFORM_TEXTURE_H_INCLUDED
#define TRANSFORM_TEXTURE_H_INCLUDED

#include "texture.h"

namespace PathTrace
{
class TransformTexture : public Texture
{
protected:
    Texture * const t;
public:
    TransformTexture(Texture * t)
        : t(t)
    {

    }
    virtual ~TransformTexture()
    {
        delete t;
    }
protected:
    virtual Vector3D transform(Vector3D v) const = 0;
public:
    virtual Color getColor(Vector3D pos) const
    {
        return t->getColor(transform(pos));
    }
    virtual float getFloat(Vector3D pos) const
    {
        return t->getFloat(transform(pos));
    }
};

class MirrorBallSkymapTexture : public TransformTexture
{
public:
    MirrorBallSkymapTexture(Texture * t)
        : TransformTexture(t)
    {
    }
    virtual Texture * duplicate() const
    {
        return new MirrorBallSkymapTexture(t->duplicate());
    }
    virtual Vector3D transform(Vector3D v) const
    {
        if(v == Vector3D(0))
            return Vector3D(0);
        v = normalize(v);
        if(v.z <= -1)
            return Vector3D(0, 0.5, 0);
        float d = std::sqrt(2 + 2 * v.z);
        if(d == 0)
            return Vector3D(0, 0.5, 0);
        float xt = v.x / d;
        float yt = v.y / d;
        return Vector3D(xt * 0.5 + 0.5, yt * 0.5 + 0.5, 0);
    }
};

class SphericalCoordinatesSkymapTexture : public TransformTexture
{
public:
    SphericalCoordinatesSkymapTexture(Texture * t)
        : TransformTexture(t)
    {
    }
    virtual Texture * duplicate() const
    {
        return new SphericalCoordinatesSkymapTexture(t->duplicate());
    }
    virtual Vector3D transform(Vector3D v) const
    {
        if(v == Vector3D(0))
            return Vector3D(0);
        v = normalize(v);
        float theta = std::atan2(v.y, v.x);
        if(theta < -M_PI)
            theta += 2 * M_PI;
        if(theta > M_PI)
            theta -= 2 * M_PI;
        float phi = asin(v.z);
        return Vector3D(theta * 0.5 / M_PI + 0.5, phi / (M_PI / 2) * 0.5 + 0.5, 0);
    }
};
}

#endif // TRANSFORM_TEXTURE_H_INCLUDED
