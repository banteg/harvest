// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IImage.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::video namespace; not the original source. The virtual order follows
// the Mac 1.18 vtable of daisy::video::CImage.

#ifndef OX_VIDEO_IIMAGE_H
#define OX_VIDEO_IIMAGE_H

#include "../IUnknown.h"
#include "../core/CDimension2d.h"
#include "SColor.h"

namespace ox {
namespace video {

//! A software image.
class IImage : public IUnknown
{
public:
    virtual void* lock() = 0;
    virtual void unlock() = 0;
    virtual const core::CDimension2d<int>& getDimension() = 0;
    virtual int getBitsPerPixel() = 0;
    virtual int getBytesPerPixel() = 0;
    virtual int getImageDataSizeInBytes() = 0;
    virtual int getImageDataSizeInPixels() = 0;
    virtual SColor getPixel(int x, int y) = 0;
    virtual int getColorFormat() = 0;
    virtual unsigned int getRedMask() = 0;
    virtual unsigned int getGreenMask() = 0;
    virtual unsigned int getBlueMask() = 0;
    virtual unsigned int getAlphaMask() = 0;
};

} // end namespace video
} // end namespace ox

#endif
