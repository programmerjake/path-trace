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
#include <fstream>
#include <cctype>

using PathTrace::Color;
using namespace std;

namespace
{
bool matches(istream & is, string v)
{
    char * buffer = new char[v.size()];
    try
    {
        if(!is.read(buffer, v.size()))
            return false;
    }
    catch(...)
    {
        delete []buffer;
        throw;
    }
    bool retval = (v == string(buffer, v.size()));
    delete []buffer;
    return retval;
}
}

Image::Image(string fileName, string format)
{
    if(format == "")
    {
        size_t index = fileName.find_last_of('.');
        if(index == string::npos)
            throw ImageLoadError("can't determine format");
        format = fileName.substr(index + 1);
        for(size_t i = 0; i < format.size(); i++)
            format[i] = tolower(format[i]);
    }
    if(format == "png")
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
#if 1
#warning finish implementing hdr load
#else
    else if(format == "hdr" || format == "pic")
    {
        ifstream is(fileName.c_str(), ios::binary);
        if(!matches(is, "#?RADIANCE\n"))
            throw ImageLoadError("magic string doesn't match");
        bool gotFormat = false;
        float scaleFactor = 1;
        while(true)
        {
            string v = "";
            char ch;
            while(true)
            {
                if(!is.get(ch))
                    throw ImageLoadError("unexpected EOF");
                if(ch == '=')
                {
                    break;
                }
                if(ch == '#')
                {
                    while(true)
                    {
                        if(!is.get(ch))
                            throw ImageLoadError("unexpected EOF");
                        if(ch == '\n')
                            break;
                    }
                }
                if(ch == ' ')
                {
                    continue;
                }
                if(ch == '\n')
                {
                    if(v != "")
                        throw ImageLoadError("unexpected token");
                    continue;
                }
                if(ch == '+' || ch == '-')
                {
                    if(v != "")
                        throw ImageLoadError("unexpected token");
                    goto at_size;
                }
                if(!isalpha(ch))
                    throw ImageLoadError("unexpected character");
                v += ch;
            }
            if(v == "FORMAT")
            {
                if(gotFormat)
                    throw ImageLoadError("format already specified");
                gotFormat = true;
                if(!matches(is, "32-bit_rle_rgbe\n"))
                    throw ImageLoadError("invalid format specifier");
            }
            else if(v == "EXPOSURE")
            {
                float exposure;
                if(!is >> exposure)
                    throw ImageLoadError("can't read exposure value");
                while(true)
                {
                    char ch;
                    if(!is.get(ch))
                        throw ImageLoadError("unexpected EOF");
                    if(ch == '\n')
                        break;
                    if(!isspace(ch))
                        throw ImageLoadError("unexpected character");
                }
                scaleFactor /= exposure;
            }
            else // unknown variable
            {
                while(true)
                {
                    char ch;
                    if(!is.get(ch))
                        throw ImageLoadError("unexpected EOF");
                    if(ch == '\n')
                        break;
                }
            }
        }
at_size:
    }
#endif
    else
        throw ImageLoadError("invalid format");
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
