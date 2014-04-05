#ifndef IMAGE_TEXTURE_H_INCLUDED
#define IMAGE_TEXTURE_H_INCLUDED

#include "image.h"
#include "texture.h"

namespace PathTrace
{
class ImageTexture : public Texture
{
private:
    Image image;
public:
    ImageTexture(Image image)
        : image(image)
    {
    }
    virtual Color getColor(Vector3D v) const
    {
        float x = v.x, y = v.y;
        x -= floor(x);
        y -= floor(y);
        y = 1 - y;
        x *= image.width();
        y *= image.height();
        int xi = (int)std::floor(x), yi = (int)std::floor(y);
        return image.getPixel(xi, yi);
    }
    virtual Texture *duplicate() const
    {
        return new ImageTexture(image);
    }
};

class ImageAlphaTexture : public Texture
{
private:
    Image image;
public:
    ImageAlphaTexture(Image image)
        : image(image)
    {
    }
    virtual Color getColor(Vector3D v) const
    {
        float x = v.x, y = v.y;
        x -= floor(x);
        y -= floor(y);
        y = 1 - y;
        x *= image.width();
        y *= image.height();
        int xi = (int)std::floor(x), yi = (int)std::floor(y);
        return Color(image.getPixelAlpha(xi, yi));
    }
    virtual float getFloat(Vector3D v) const
    {
        float x = v.x, y = v.y;
        x -= floor(x);
        y -= floor(y);
        y = 1 - y;
        x *= image.width();
        y *= image.height();
        int xi = (int)std::floor(x), yi = (int)std::floor(y);
        return image.getPixelAlpha(xi, yi);
    }
    virtual Texture *duplicate() const
    {
        return new ImageAlphaTexture(image);
    }
};

class ImageSkyboxTexture : public Texture
{
private:
    Image top, bottom, left, right, front, back;
    static Color getColor(float x, float y, Image image)
    {
        x = x * 0.5 + 0.5;
        y = 0.5 - y * 0.5;
        x *= image.width();
        y *= image.height();
        int xi = (int)std::floor(x), yi = (int)std::floor(y);
        return image.getPixel(xi, yi);
    }
public:
    ImageSkyboxTexture(Image top, Image bottom, Image left, Image right, Image front, Image back)
        : top(top), bottom(bottom), left(left), right(right), front(front), back(back)
    {
    }
    virtual Color getColor(Vector3D v) const
    {
        if(v == Vector3D(0))
            return Color(0);
        Vector3D a = Vector3D(std::abs(v.x), std::abs(v.y), std::abs(v.z));
        if(a.x > a.y && a.x > a.z)
        {
            if(v.x < 0)
                return getColor(-v.z / a.x, v.y / a.x, left);
            return getColor(v.z / a.x, v.y / a.x, right);
        }
        if(a.y > a.z)
        {
            if(v.y < 0)
                return getColor(-v.x / a.y, v.z / a.y, bottom);
            return getColor(v.x / a.y, v.z / a.y, top);
        }
        if(v.z < 0)
            return getColor(v.x / a.z, v.y / a.z, back);
        return getColor(-v.x / a.z, v.y / a.z, front);
    }
    virtual Texture *duplicate() const
    {
        return new ImageSkyboxTexture(top, bottom, left, right, front, back);
    }
};

class ImageSkyboxAlphaTexture : public Texture
{
private:
    Image top, bottom, left, right, front, back;
    static float get(float x, float y, Image image)
    {
        x = x * 0.5 + 0.5;
        y = 0.5 - y * 0.5;
        x *= image.width();
        y *= image.height();
        int xi = (int)std::floor(x), yi = (int)std::floor(y);
        return image.getPixelAlpha(xi, yi);
    }
public:
    ImageSkyboxAlphaTexture(Image top, Image bottom, Image left, Image right, Image front, Image back)
        : top(top), bottom(bottom), left(left), right(right), front(front), back(back)
    {
    }
    virtual float getFloat(Vector3D v) const
    {
        if(v == Vector3D(0))
            return 0;
        Vector3D a = Vector3D(std::abs(v.x), std::abs(v.y), std::abs(v.z));
        if(a.x > a.y && a.x > a.z)
        {
            if(v.x < 0)
                return get(-v.z / a.x, v.y / a.x, left);
            return get(v.z / a.x, v.y / a.x, right);
        }
        if(a.y > a.z)
        {
            if(v.y < 0)
                return get(-v.x / a.y, v.z / a.y, bottom);
            return get(v.x / a.y, v.z / a.y, top);
        }
        if(v.z < 0)
            return get(v.x / a.z, v.y / a.z, back);
        return get(-v.x / a.z, v.y / a.z, front);
    }
    virtual Color getColor(Vector3D v) const
    {
        if(v == Vector3D(0))
            return 0;
        Vector3D a = Vector3D(std::abs(v.x), std::abs(v.y), std::abs(v.z));
        if(a.x > a.y && a.x > a.z)
        {
            if(v.x < 0)
                return Color(get(-v.z / a.x, v.y / a.x, left));
            return Color(get(v.z / a.x, v.y / a.x, right));
        }
        if(a.y > a.z)
        {
            if(v.y < 0)
                return Color(get(-v.x / a.y, v.z / a.y, bottom));
            return Color(get(v.x / a.y, v.z / a.y, top));
        }
        if(v.z < 0)
            return Color(get(v.x / a.z, v.y / a.z, back));
        return Color(get(-v.x / a.z, v.y / a.z, front));
    }
    virtual Texture *duplicate() const
    {
        return new ImageSkyboxAlphaTexture(top, bottom, left, right, front, back);
    }
};
}

#endif // IMAGE_TEXTURE_H_INCLUDED
