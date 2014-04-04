#ifndef TEXTURE_H_INCLUDED
#define TEXTURE_H_INCLUDED

#include "color.h"
#include "vector3d.h"
#include "transform.h"

namespace PathTrace
{
class Texture
{
public:
    virtual Color getColor(Vector3D pos) const = 0;
    virtual float getFloat(Vector3D pos) const
    {
        Color c = getColor(pos);
        return (c.x + c.y + c.z) * (1.0f / 3.0f);
    }
    virtual Texture *duplicate() const = 0;
    virtual Texture *transform(const Matrix &m) const
    {
        return NULL;
    }
    virtual ~Texture()
    {
    }
};

class ColorTexture : public Texture
{
private:
    Color color;
public:
    ColorTexture(Color color)
        : color(color)
    {
    }
    ColorTexture(float r, float g, float b)
        : color(r, g, b)
    {
    }
    ColorTexture(float v)
        : color(v)
    {
    }
    virtual Color getColor(Vector3D) const
    {
        return color;
    }
    virtual Texture *duplicate() const
    {
        return new ColorTexture(color);
    }
    virtual Texture *transform(const Matrix &) const
    {
        return new ColorTexture(color);
    }
};

class TransformedTexture : public Texture
{
private:
    Matrix m;
    Texture *t;
public:
    TransformedTexture(const Matrix &m, Texture *t)
        : m(m), t(t)
    {
    }
    virtual ~TransformedTexture()
    {
        delete t;
    }
    virtual Color getColor(Vector3D v) const
    {
        return t->getColor(PathTrace::transform(m, v));
    }
    virtual float getFloat(Vector3D v) const
    {
        return t->getFloat(PathTrace::transform(m, v));
    }
    virtual Texture *duplicate() const
    {
        return new TransformedTexture(m, t->duplicate());
    }
    virtual Texture *transform(const Matrix &m) const
    {
        return new TransformedTexture(this->m.concat(m), t->duplicate());
    }
};

inline Texture *transform(const Matrix &m, Texture *t)
{
    Texture *retval = t->transform(m);
    if(!retval)
        retval = new TransformedTexture(m, t->duplicate());
    return retval;
}
}

#endif // TEXTURE_H_INCLUDED
