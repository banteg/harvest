// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CColorConverter.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest from the Mac and Linux builds; not the original source.
// allow: SIZE_OK - Keep the original translation unit intact for matching function order and compiler context.

#include "CColorConverter.h"
#include "ox/video/ColorPacking.h"
// The native object includes an iostream static initializer.
#include <iostream> // IWYU pragma: keep

namespace daisy {
namespace video {

void CColorConverter::convert4BitTo16BitFlipMirror(const char* in, short* out, int width, int height,
                                                int pitch, const int* palette)
{
    int shift = 0;
    out += width * height;
    short* oout = out;
    for (int y = 0; y < height; ++y)
    {
        shift = 4;
        out = oout - y * width - width;
        for (int x = 0; x < width; ++x)
        {
            *out = ox::video::X8R8G8B8toA1R5G5B5(palette[(unsigned char)((*in >> shift) & 0xf)]);
            ++out;
            shift -= 4;
            if (shift < 0)
            {
                shift = 4;
                ++in;
            }
        }
        if (shift != 4)
            ++in;
        in += pitch;
    }
}

void CColorConverter::convert8BitTo16Bit(const char* in, short* out, int width, int height,
                                      int pitch, const int* palette)
{
    int lineWidth = width + pitch;
    const char* p = in;
    for (int y = 1; y <= height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            *out = ox::video::X8R8G8B8toA1R5G5B5(palette[(unsigned char)(*p)]);
            ++out;
            ++p;
        }
        p = in + y * lineWidth + pitch;
    }
}

void CColorConverter::convert8BitTo16BitFlipMirror(const char* in, short* out, int width, int height,
                                                int pitch, const int* palette)
{
    out += width * height;
    int lineWidth = width + pitch;
    const char* p = in;
    for (int y = 1; y <= height; ++y)
    {
        p = in + lineWidth * y - pitch;
        for (int x = 0; x < width; ++x)
        {
            --out;
            --p;
            *out = ox::video::X8R8G8B8toA1R5G5B5(palette[(unsigned char)(*p)]);
        }
    }
}

void CColorConverter::convert1BitTo16BitFlipMirror(const char* in, short* out, int width, int height,
                                                int pitch)
{
    short* p = out + width * height;
    for (int y = 0; y < height; ++y)
    {
        int shift = 7;
        out = p - y * width - width;
        for (int x = 0; x < width; ++x)
        {
            *out = *in >> shift & 1 ? (short)0xffff : (short)0;
            ++out;
            --shift;
            if (shift < 0)
            {
                shift = 7;
                ++in;
            }
        }
        if (shift != 7)
            ++in;
        in += pitch;
    }
}

void CColorConverter::convert16BitTo16BitFlipMirror(const short* in, short* out, int width, int height,
                                                 int pitch)
{
    const short* p = in;
    const int lineWidth = width + pitch;
    out += width * height;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            p = in + width - x - 1;
            --out;
            *out = *p;
        }
        in += lineWidth;
    }
}

void CColorConverter::convert24BitTo16BitFlipMirror(const char* in, short* out, int width, int height,
                                                 int pitch)
{
    const char* p = in;
    const int lineWidth = 3 * width + pitch;
    out += width * height;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            p = in + (width - x - 1) * 3;
            --out;
            *out = ox::video::RGB16(p[2], p[1], *p);
        }
        in += lineWidth;
    }
}

void CColorConverter::convert24BitTo24BitFlipMirrorColorShuffle(const char* in, char* out, int width,
                                                             int height, int pitch)
{
    const char* p = in;
    const int lineWidth = 3 * width + pitch;
    out += width * height * 3;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            p = in + (width - x - 1) * 3;
            out -= 3;
            out[0] = p[2];
            out[1] = p[1];
            out[2] = p[0];
        }
        in += lineWidth;
    }
}

void CColorConverter::convert24BitTo16BitColorShuffle(const char* in, short* out, int width, int height,
                                                   int pitch)
{
    const char* p = in;
    const int lineWidth = 3 * width + pitch;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            p = in + (width - x - 1) * 3;
            *out = ox::video::RGB16(*p, p[1], p[2]);
            ++out;
        }
        in += lineWidth;
    }
}

void CColorConverter::convert24BitTo16BitFlipColorShuffle(const char* in, short* out, int width,
                                                       int height, int pitch)
{
    const char* p = in;
    const int lineWidth = 3 * width + pitch;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            p = in + x * 3;
            *out = ox::video::RGB16(*p, p[1], p[2]);
            ++out;
        }
        in += lineWidth;
    }
}

void CColorConverter::convert32BitTo16BitColorShuffle(const char* in, short* out, int width, int height,
                                                   int pitch)
{
    const char* p = in;
    const int lineWidth = 4 * width + pitch;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            p = in + (width - x - 1) * 4;
            *out = ox::video::RGB16(p[2], p[1], p[0]);
            ++out;
        }
        in += lineWidth;
    }
}

void CColorConverter::convert32BitTo16BitFlipMirrorColorShuffle(const char* in, short* out, int width,
                                                             int height, int pitch)
{
    const char* p = in;
    const int lineWidth = 4 * width + pitch;
    out += (width + pitch) * height;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            p = in + (width - x - 1) * 4;
            --out;
            *out = ox::video::RGB16(p[2], p[1], p[0]);
        }
        in += lineWidth;
    }
}

void CColorConverter::convert16bitToA8R8G8B8andResize(const short* in, int* out, int newWidth,
                                                  int newHeight, int currentWidth, int currentHeight)
{
    if (!newWidth || !newHeight)
        return;
    float sourceXStep = (float)currentWidth / (float)newWidth;
    float sourceYStep = (float)currentHeight / (float)newHeight;
    float sy;
    int t;
    for (int x = 0; x < newWidth; ++x)
    {
        sy = 0.0f;
        for (int y = 0; y < newHeight; ++y)
        {
            t = in[(int)(((int)sy) * currentWidth + x * sourceXStep)];
            t = (((t >> 15) & 1) << 31) | (((t >> 10) & 0x1f) << 19)
                | (((t >> 5) & 0x1f) << 11) | ((t & 0x1f) << 3);
            out[y * newWidth + x] = t;
            sy += sourceYStep;
        }
    }
}

void CColorConverter::convert32BitTo32BitFlipMirror(const int* in, int* out, int width, int height,
                                                 int pitch)
{
    const int* p = in;
    const int lineWidth = width;
    out += height * width;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            p = in + width - x - 1;
            --out;
            *out = *p;
        }
        in += lineWidth;
    }
}

void CColorConverter::convert32BitTo32Bit(const int* in, int* out, int count,
                                        ox::video::ECOLOR_FORMAT input, ox::video::ECOLOR_FORMAT output)
{
    int inA = (unsigned int)input >> 24;
    int inR = ((int)input & 0xff0000) >> 16;
    int inG = ((int)input & 0xff00) >> 8;
    int inB = (int)input & 0xff;
    int outA = (unsigned int)output >> 24;
    int outR = ((int)output & 0xff0000) >> 16;
    int outG = ((int)output & 0xff00) >> 8;
    int outB = (int)output & 0xff;
    for (int i = 0; i < count; ++i)
    {
        unsigned int color = in[i];
        out[i] = ((color << inA) & 0xff000000) >> outA | ((color << inR) & 0xff000000) >> outR
            | ((color << inB) & 0xff000000) >> outB | ((color << inG) & 0xff000000) >> outG;
    }
}

int CColorConverter::convert32BitTo32Bit(int color, ox::video::ECOLOR_FORMAT input,
                                       ox::video::ECOLOR_FORMAT output)
{
    if (input == output)
        return color;
    return (((unsigned int)color << (((unsigned int)input & 0xff000000) >> 24)) & 0xff000000)
            >> (((unsigned int)output & 0xff000000) >> 24)
        | (((unsigned int)color << (((int)input & 0xff0000) >> 16)) & 0xff000000)
            >> (((int)output & 0xff0000) >> 16)
        | (((unsigned int)color << ((int)input & 0xff)) & 0xff000000) >> ((int)output & 0xff)
        | (((unsigned int)color << (((int)input & 0xff00) >> 8)) & 0xff000000)
            >> (((int)output & 0xff00) >> 8);
}

} // end namespace video
} // end namespace daisy
