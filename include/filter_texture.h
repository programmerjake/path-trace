#ifndef FILTER_TEXTURE_H_INCLUDED
#define FILTER_TEXTURE_H_INCLUDED

#include "texture.h"

namespace PathTrace
{
class FilterTexture : public Texture
{
protected:
    Texture * const t;
public:
    FilterTexture(Texture * t)
        : t(t)
    {
    }
    virtual ~FilterTexture()
    {
        delete t;
    }
protected:
    virtual Color filter(Color v) const = 0;
public:
    virtual Color getColor(Vector3D pos) const
    {
        return filter(t->getColor(pos));
    }
};

class MultiplyTexture : public FilterTexture
{
private:
    Color factor;
public:
    MultiplyTexture(Color factor, Texture * t)
        : FilterTexture(t), factor(factor)
    {
    }
    virtual Texture * duplicate() const
    {
        return new MultiplyTexture(factor, t->duplicate());
    }
protected:
    virtual Color filter(Color v) const
    {
        return v * factor;
    }
};

class LogTexture : public FilterTexture
{
public:
    LogTexture(Texture * t)
        : FilterTexture(t)
    {
    }
    virtual Texture * duplicate() const
    {
        return new LogTexture(t->duplicate());
    }
private:
    static float myLog(float v)
    {
        if(v <= 1e-30)
            return 0;
        return 0.5f + (float)std::log(v) / (float)std::log(2) / 256;
    }
protected:
    virtual Color filter(Color v) const
    {
        return Color(myLog(v.x), myLog(v.y), myLog(v.z));
    }
};
}

#endif // FILTER_TEXTURE_H_INCLUDED
