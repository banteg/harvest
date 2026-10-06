// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CImage.cpp for the Harvest port; see
// third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
//
// daisy::video::CImage as the original uses it: Irrlicht 0.7's image with daisy's A8R8G8B8 value
// (0x08101800). Only the members src/daisy/video/Null/CImage.h declares; the behaviour follows the
// Linux build (getPixel 0x4dd460, getBitsPerPixelFromFormat 0x4ddda0, initData 0x4ddea0).

#include "daisy/video/Null/CImage.h"
#include "daisy/os.h"
#include "video/ColorFormats.h"

namespace daisy {
namespace video {

using namespace ox::video;

CImage::CImage(ECOLOR_FORMAT format, const ox::core::CDimension2d<int>& size)
    : Data(0), Size(size), Format(format)
{
    initData();
}

//! Takes ownership of data without allocating, even when data is null (as Irrlicht 0.7 does).
CImage::CImage(ECOLOR_FORMAT format, const ox::core::CDimension2d<int>& size, void* data)
    : Data(data), Size(size), Format(format)
{
    BitsPerPixel = getBitsPerPixelFromFormat();
    BytesPerPixel = BitsPerPixel / 8;
}

CImage::~CImage()
{
    delete[] (char*)Data;
}

void CImage::initData()
{
    BitsPerPixel = getBitsPerPixelFromFormat();
    BytesPerPixel = BitsPerPixel / 8;

    if (!Data)
        Data = new char[Size.Height * Size.Width * BytesPerPixel];
}

int CImage::getBitsPerPixelFromFormat()
{
    switch ((int)Format)
    {
    case ECF_A1R5G5B5:
        AlphaMask = 0x1 << 15;
        RedMask = 0x1f << 10;
        GreenMask = 0x1f << 5;
        BlueMask = 0x1f;
        return 16;
    case port::ECF_R5G6B5:
        AlphaMask = 0;
        RedMask = 0x1f << 11;
        GreenMask = 0x3f << 5;
        BlueMask = 0x1f;
        return 16;
    case port::ECF_R8G8B8:
        AlphaMask = 0;
        RedMask = 0xff << 16;
        GreenMask = 0xff << 8;
        BlueMask = 0xff;
        return 24;
    case ECF_A8R8G8B8:
        AlphaMask = 0xffu << 24;
        RedMask = 0xff << 16;
        GreenMask = 0xff << 8;
        BlueMask = 0xff;
        return 32;
    }

    os::Printer::log("CImage: Unknown color format.", ox::event::ELL_ERROR);
    return 0;
}

void* CImage::lock()
{
    return Data;
}

void CImage::unlock()
{
}

const ox::core::CDimension2d<int>& CImage::getDimension()
{
    return Size;
}

int CImage::getBitsPerPixel()
{
    return BitsPerPixel;
}

int CImage::getBytesPerPixel()
{
    return BytesPerPixel;
}

int CImage::getImageDataSizeInBytes()
{
    return BytesPerPixel * Size.Width * Size.Height;
}

int CImage::getImageDataSizeInPixels()
{
    return Size.Width * Size.Height;
}

//! The pixel as A8R8G8B8; 0 outside the image or for an unknown format. Unlike Irrlicht 0.7, the
//! Linux build has no R5G6B5 case: those pixels read as 0 too.
SColor CImage::getPixel(int x, int y)
{
    if (x < 0 || y < 0 || x >= Size.Width || y >= Size.Height)
        return SColor(0);

    switch ((int)Format)
    {
    case ECF_A1R5G5B5:
        return SColor(port::A1R5G5B5toA8R8G8B8(((unsigned short*)Data)[y * Size.Width + x]));
    case ECF_A8R8G8B8:
        return SColor(((unsigned int*)Data)[y * Size.Width + x]);
    case port::ECF_R8G8B8:
    {
        unsigned char* p = &((unsigned char*)Data)[(y * Size.Width + x) * 3];
        return SColor(255, p[0], p[1], p[2]);
    }
    }

    return SColor(0);
}

int CImage::getColorFormat()
{
    return Format;
}

unsigned int CImage::getRedMask()
{
    return RedMask;
}

unsigned int CImage::getGreenMask()
{
    return GreenMask;
}

unsigned int CImage::getBlueMask()
{
    return BlueMask;
}

unsigned int CImage::getAlphaMask()
{
    return AlphaMask;
}

} // end namespace video
} // end namespace daisy
