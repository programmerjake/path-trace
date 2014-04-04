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
#include "image.h"
#include "png_decoder.h"
#include <cstring>
#include <iostream>

using PathTrace::Color;

Image::Image(string fileName)
{
    try
    {
        PngDecoder decoder(fileName);
        const size_t arraySize = FloatsPerPixel * decoder.width() * decoder.height();
        float *newArray = new float[arraySize];
        uint8_t *array = decoder.removeData();
        for(size_t i = 0; i < arraySize; i++)
        {
            newArray[i] = (int)array[i] / 255.0f;
        }
        delete []array;
        data = new data_t(newArray, decoder.width(), decoder.height());
    }
    catch(PngLoadError &e)
    {
        throw ImageLoadError(e.what());
    }
}

Image::Image()
    : data(NULL)
{
}

Image::~Image()
{
    if(data)
    {
        if(data->refCount-- <= 0)
            delete data;
    }
}

Image::Image(const Image & rt)
{
    data = rt.data;
    if(data)
        data->refCount++;
}

const Image & Image::operator =(const Image & rt)
{
    if(data == rt.data)
        return *this;
    if(data)
    {
        if(data->refCount-- <= 0)
            delete data;
    }
    data = rt.data;
    if(data)
        data->refCount++;
    return *this;
}

Color Image::getPixel(int x, int y) const
{
    if(!data)
    {
        return Color();
    }

    if(y < 0 || (unsigned)y >= data->h || x < 0 || (unsigned)x >= data->w)
    {
        return Color();
    }

    float *pixel = &data->data[FloatsPerPixel * (x + y * data->w)];
    return Color(pixel[0], pixel[1], pixel[2]);
}

float Image::getPixelAlpha(int x, int y) const
{
    if(!data)
    {
        return 0;
    }

    if(y < 0 || (unsigned)y >= data->h || x < 0 || (unsigned)x >= data->w)
    {
        return 0;
    }

    float *pixel = &data->data[FloatsPerPixel * (x + y * data->w)];
    return pixel[3];
}
