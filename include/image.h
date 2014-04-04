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
};

#endif // IMAGE_H
