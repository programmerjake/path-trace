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
#if 0
#warning finish implementing hdr load
#else
    else if(format == "hdr" || format == "pic")
    {
        ifstream is(fileName.c_str(), ios::binary);
        if(!matches(is, "#?RADIANCE\n"))
            throw ImageLoadError("magic string doesn't match");
        bool gotFormat = false;
        Color scaleFactor = Color(1);
        char ch;
        while(true)
        {
            string v = "";
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
                if(!(is >> exposure))
                    throw ImageLoadError("can't read exposure value");
                while(true)
                {
                    if(!is.get(ch))
                        throw ImageLoadError("unexpected EOF");
                    if(ch == '\n')
                        break;
                    if(!isspace(ch))
                        throw ImageLoadError("unexpected character");
                }
                scaleFactor /= exposure;
            }
            else if(v == "COLORCORR")
            {
                float r, g, b;
                if(!(is >> r >> g >> b))
                    throw ImageLoadError("can't read color correction values");
                while(true)
                {
                    if(!is.get(ch))
                        throw ImageLoadError("unexpected EOF");
                    if(ch == '\n')
                        break;
                    if(!isspace(ch))
                        throw ImageLoadError("unexpected character");
                }
                scaleFactor /= Color(r, g, b);
            }
            else // unknown variable
            {
                while(true)
                {
                    if(!is.get(ch))
                        throw ImageLoadError("unexpected EOF");
                    if(ch == '\n')
                        break;
                }
            }

        }
at_size:
        if(ch != '-')
            throw ImageLoadError("invalid resolution string");
        if(!matches(is, "Y"))
            throw ImageLoadError("invalid resolution string");
        int w, h;
        if(!(is >> h) || h <= 0)
            throw ImageLoadError("invalid resolution string");
        while(true)
        {
            if(!is.get(ch))
                throw ImageLoadError("unexpected EOF");
            if(ch != ' ' && ch != '\t')
                break;
        }
        if(ch != '+' || !matches(is, "X"))
            throw ImageLoadError("invalid resolution string");
        if(!(is >> w) || w <= 0 || w >= 1 << 15)
            throw ImageLoadError("invalid resolution string");
        while(true)
        {
            if(!is.get(ch))
                throw ImageLoadError("unexpected EOF");
            if(ch == '\n')
                break;
            if(!isspace(ch))
                throw ImageLoadError("unexpected character");
        }
        uint8_t * rgbe = new uint8_t[4 * w * h];
        float * image = new float[FloatsPerPixel * w * h];
        try
        {
            for(size_t y = 0; y < (size_t)h; y++)
            {
                uint8_t * line = &rgbe[4 * (size_t)w * (size_t)y];
                if(!is.read((char *)line, 4))
                    throw ImageLoadError("unexpected EOF");
                bool useOldDecompress = false;
                if(line[0] != 2 || line[1] != 2 || line[2] & 0x80)
                    useOldDecompress = true;
                else if((line[2] << 8) + line[3] != w)
                    throw ImageLoadError("invalid line length in new compressed line");
                if(!useOldDecompress)
                {
                    for(int component = 0; component < 4; component++)
                    {
                        ios::pos_type lastRun = (ios::pos_type)-1;
                        for(size_t x = 0; x < (size_t)w;)
                        {
                            uint8_t byte;
                            ios::pos_type runStart = is.tellg();
                            if(!is.get((char &)byte))
                                throw ImageLoadError("unexpected EOF");
                            if(byte > 0x80)
                            {
                                size_t count = byte - 0x80;
                                if(!is.get((char &)byte))
                                    throw ImageLoadError("unexpected EOF");
                                for(size_t i = 0; i < count; i++)
                                {
                                    if(x >= (size_t)w)
                                    {
                                        cout << "pos : 0x" << hex << runStart << " 0x" << lastRun << dec << endl;
                                        throw ImageLoadError("line too long");
                                    }
                                    line[component + 4 * x++] = byte;
                                }
                            }
                            else
                            {
                                size_t count = byte;
                                for(size_t i = 0; i < count; i++)
                                {
                                    if(x >= (size_t)w)
                                        throw ImageLoadError("line too long");
                                    if(!is.get((char &)byte))
                                        throw ImageLoadError("unexpected EOF");
                                    line[component + 4 * x++] = byte;
                                }
                            }
                            lastRun = runStart;
                        }
                    }
                }
                else
                {
                    size_t x = 0;
                    for(;;)
                    {
                        size_t repCount = 0;
                        int shift = 0;
                        while(line[0] == 1 && line[1] == 1 && line[2] == 1)
                        {
                            if(shift >= 16)
                                throw ImageLoadError("too many bytes in repeat count");
                            repCount += (size_t)line[3] << shift;
                            shift += 8;
                        }
                        if(shift != 0 && (repCount == 0 || x + repCount > (size_t)w))
                            throw ImageLoadError("invalid repeat count");
                        if(shift != 0)
                        {
                            if(!is.read((char *)line, 4))
                                throw ImageLoadError("unexpected EOF");
                            for(size_t i = 1; i < repCount; i++)
                            {
                                line[4] = line[0];
                                line[5] = line[1];
                                line[6] = line[2];
                                line[7] = line[3];
                                line += 4;
                                x++;
                            }
                        }
                        line += 4;
                        x++;
                        if(x >= (size_t)w)
                            break;
                    }
                }
            }
            is.close();
            for(size_t i = 0; i < 4 * (size_t)w * (size_t)h; i += 4)
            {
                int exponent = rgbe[i + 3] - 128;
                float factor = 179.0f * (float)std::pow(2.0f, exponent - 8);
                image[i + 0] = rgbe[i + 0] * factor * scaleFactor.x;
                image[i + 1] = rgbe[i + 1] * factor * scaleFactor.y;
                image[i + 2] = rgbe[i + 2] * factor * scaleFactor.z;
                image[i + 3] = 1;
            }
            data = new data_t(image, w, h);
        }
        catch(...)
        {
            delete []rgbe;
            delete []image;
            throw;
        }
        delete []rgbe;
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

void MutableImage::writeHDR(string fileName) const
{
    uint8_t * scanLine = new uint8_t[w * 4];
    try
    {
        ofstream os(fileName.c_str(), ios::binary);
        if(!os)
            throw ImageStoreError("can't open file for writing");
        os << "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y " << h << " +X " << w << "\n";
        uint8_t lineCode[4] = {2, 2, (uint8_t)(w >> 8), (uint8_t)(w & 0xFF)};
        for(size_t y = 0; y < h; y++)
        {
            if(!os.write((const char *)lineCode, sizeof(lineCode)))
                throw ImageStoreError("can't write to file");
            for(size_t x = 0, index = FloatsPerPixel * (x + y * (size_t)w); x < w; x++, index += FloatsPerPixel)
            {
                float maxV = max(data[index], max(data[index + 1], data[index + 2])) / 179.0f;
                if(maxV < 1e-30)
                {
                    scanLine[4 * x + 0] = 0;
                    scanLine[4 * x + 1] = 0;
                    scanLine[4 * x + 2] = 0;
                    scanLine[4 * x + 3] = 0;
                    continue;
                }
                int lg = (int)ceil(log(maxV) / log(2) + 1e-5);
                float scale = pow(0.5, lg - 8) / 179.0f;
                scanLine[4 * x + 0] = max(0, min(0xFF, (int)floor(data[index + 0] * scale)));
                scanLine[4 * x + 1] = max(0, min(0xFF, (int)floor(data[index + 1] * scale)));
                scanLine[4 * x + 2] = max(0, min(0xFF, (int)floor(data[index + 2] * scale)));
                scanLine[4 * x + 3] = lg + 128;
            }
            char buffer[0x80];
            for(size_t channel = 0; channel < 4; channel++)
            {
                size_t currentRunLength = 0, skipCount = 0;
                for(size_t x = 0; x < w;)
                {
                    while(currentRunLength < 0x7F && skipCount <= 0x80 && currentRunLength + skipCount + x < w)
                    {
                        while(currentRunLength < 0x7F && currentRunLength + skipCount + x < w && scanLine[channel + 4 * (x + skipCount)] == scanLine[channel + 4 * (x + skipCount + currentRunLength)])
                            currentRunLength++;
                        if(currentRunLength < 3)
                        {
                            skipCount += currentRunLength;
                            currentRunLength = 0;
                        }
                        else
                            break;
                    }
                    assert(currentRunLength <= 0x7F && currentRunLength + x + skipCount <= w);
                    if(currentRunLength > 0)
                        assert(skipCount <= 0x80);
                    else if(skipCount > 0x80)
                        skipCount = 0x80;
                    if(skipCount > 0)
                    {
                        os.put((char)(uint8_t)skipCount);
                        for(size_t i = 0; i < skipCount; i++)
                        {
                            buffer[i] = scanLine[channel + 4 * (x + i)];
                        }
                        os.write(buffer, skipCount);
                        x += skipCount;
                        skipCount = 0;
                    }
                    if(currentRunLength > 0)
                    {
                        os.put((char)(uint8_t)(currentRunLength + 0x80));
                        os.put((char)scanLine[channel + 4 * (x + skipCount)]);
                        x += currentRunLength;
                        currentRunLength = 0;
                    }
                }
            }
        }
    }
    catch(...)
    {
        delete []scanLine;
        throw;
    }
    delete []scanLine;
}
