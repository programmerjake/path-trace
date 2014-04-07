/*
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>
#include <string>
#include <stdexcept>
#include "mutex.h"
#include "atomic.h"
#include "color.h"

using namespace std;

class ImageLoadError : public runtime_error
{
public:
    explicit ImageLoadError(const string &arg)
        : runtime_error(arg)
    {
    }
};

class ImageStoreError : public runtime_error
{
public:
    explicit ImageStoreError(const string &arg)
        : runtime_error(arg)
    {
    }
};

class Image
{
public:
    explicit Image(string fileName, string format = "");
    Image();
    ~Image();
    Image(const Image & rt);
    const Image & operator =(const Image & rt);

    PathTrace::Color getPixel(int x, int y) const;
    float getPixelAlpha(int x, int y) const;
    unsigned width() const
    {
        return data->w;
    }
    unsigned height() const
    {
        return data->h;
    }
    operator bool() const
    {
        return data != NULL;
    }
    bool operator !() const
    {
        return data == NULL;
    }
    friend bool operator ==(Image l, Image r)
    {
        return l.data == r.data;
    }
    friend bool operator !=(Image l, Image r)
    {
        return l.data != r.data;
    }
private:
    struct data_t
    {
        float * const data;
        const unsigned w, h;
        atomic_uint refCount;
        data_t(float * data, unsigned w, unsigned h)
            : data(data), w(w), h(h), refCount(0)
        {
        }
        ~data_t()
        {
            delete []data;
        }
    };
    data_t * data;
    enum {FloatsPerPixel = 4};
    friend class MutableImage;
};

class MutableImage
{
private:
    float * data;
    unsigned w, h;
    enum {FloatsPerPixel = 4};
public:
    MutableImage(unsigned w, unsigned h)
        : data(new float[(size_t)FloatsPerPixel * w * h]), w(w), h(h)
    {
        assert(w > 0 && h > 0);
        for(size_t i = 0; i < (size_t)FloatsPerPixel * w * h; i++)
        {
            data[i] = 0;
        }
    }
    explicit MutableImage(Image img)
    {
        if(!img)
            throw runtime_error("can't create MutableImage from empty Image");
        w = img.width();
        h = img.height();
        data = new float[(size_t)FloatsPerPixel * w * h];
        for(size_t i = 0; i < (size_t)FloatsPerPixel * w * h; i++)
        {
            data[i] = img.data->data[i];
        }
    }
    MutableImage(const MutableImage & rt)
        : data(new float[(size_t)FloatsPerPixel * rt.w * rt.h]), w(rt.w), h(rt.h)
    {
        for(size_t i = 0; i < (size_t)FloatsPerPixel * w * h; i++)
        {
            data[i] = rt.data[i];
        }
    }
    ~MutableImage()
    {
        delete []data;
    }
    const MutableImage & operator =(const MutableImage & rt)
    {
        if(data == rt.data)
            return *this;
        if(w != rt.w || h != rt.h)
        {
            delete []data;
            data = new float[(size_t)FloatsPerPixel * rt.w * rt.h];
        }
        w = rt.w;
        h = rt.h;
        for(size_t i = 0; i < (size_t)FloatsPerPixel * w * h; i++)
        {
            data[i] = rt.data[i];
        }
        return *this;
    }
    PathTrace::Color getPixel(int x, int y) const
    {
        if(y < 0 || (unsigned)y >= h || x < 0 || (unsigned)x >= w)
        {
            return PathTrace::Color();
        }

        float *pixel = &data[FloatsPerPixel * (x + y * (size_t)w)];
        return PathTrace::Color(pixel[0], pixel[1], pixel[2]);
    }
    PathTrace::Color getPixelAlpha(int x, int y) const
    {
        if(y < 0 || (unsigned)y >= h || x < 0 || (unsigned)x >= w)
        {
            return PathTrace::Color();
        }

        float *pixel = &data[FloatsPerPixel * (x + y * (size_t)w)];
        return pixel[3];
    }
    void setPixel(int x, int y, PathTrace::Color c, float alpha = 1)
    {
        if(y < 0 || (unsigned)y >= h || x < 0 || (unsigned)x >= w)
        {
            return;
        }

        float *pixel = &data[FloatsPerPixel * (x + y * (size_t)w)];
        pixel[0] = c.x;
        pixel[1] = c.y;
        pixel[2] = c.z;
        pixel[3] = alpha;
    }
    unsigned width() const
    {
        return w;
    }
    unsigned height() const
    {
        return h;
    }
    operator Image() const
    {
        Image retval;
        float * newData = new float[(size_t)FloatsPerPixel * w * h];
        for(size_t i = 0; i < (size_t)FloatsPerPixel * w * h; i++)
        {
            newData[i] = data[i];
        }
        retval.data = new Image::data_t(newData, w, h);
        return retval;
    }
    void writeHDR(string fileName) const;
};

#endif // IMAGE_H
