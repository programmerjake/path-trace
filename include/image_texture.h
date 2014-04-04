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
}

#endif // IMAGE_TEXTURE_H_INCLUDED
