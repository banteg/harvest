// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSoftwareTexture.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.
// Partial: only the constructor CVideoNull uses is recovered; the tail keeps the Linux object size.

#ifndef DAISY_VIDEO_CSOFTWARETEXTURE_H
#define DAISY_VIDEO_CSOFTWARETEXTURE_H

#include "ox/video/IImage.h"
#include "ox/video/ITexture.h"

namespace daisy {
namespace video {

//! A texture of the software driver, held as a software image.
class CSoftwareTexture : public ox::video::ITexture
{
public:
    CSoftwareTexture(ox::video::IImage* surface);
    virtual ~CSoftwareTexture();

    virtual void* lock();
    virtual void unlock();
    virtual const ox::core::CDimension2d<int>& getOriginalSize();
    virtual const ox::core::CDimension2d<int>& getSize();
    virtual int getDriverType();
    virtual int getColorFormat();
    virtual int getPitch();

private:
    char Unrecovered[0x30 - sizeof(ox::video::ITexture)];
};

} // end namespace video
} // end namespace daisy

#endif
