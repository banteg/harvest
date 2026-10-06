// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only the constructor that allocates an empty image is declared; the members are not
// recovered, the tail keeps the Linux object size.

#ifndef DAISY_VIDEO_NULL_CIMAGE_H
#define DAISY_VIDEO_NULL_CIMAGE_H

#include "ox/video/IImage.h"
#include "ox/video/IVideoDriver.h"

namespace daisy {
namespace video {

//! A software image.
class CImage : public ox::video::IImage
{
public:
    //! Allocates an uninitialized image of the given format and size.
    CImage(ox::video::ECOLOR_FORMAT format, const ox::core::CDimension2d<int>& size);
    //! Creates an image over data, which it takes ownership of (freed with delete[]).
    CImage(ox::video::ECOLOR_FORMAT format, const ox::core::CDimension2d<int>& size, void* data);
    virtual ~CImage();

    virtual void* lock();
    virtual void unlock();
    virtual const ox::core::CDimension2d<int>& getDimension();
    virtual int getBitsPerPixel();
    virtual int getBytesPerPixel();
    virtual int getImageDataSizeInBytes();
    virtual int getImageDataSizeInPixels();
    virtual ox::video::SColor getPixel(int x, int y);
    virtual int getColorFormat();
    virtual unsigned int getRedMask();
    virtual unsigned int getGreenMask();
    virtual unsigned int getBlueMask();
    virtual unsigned int getAlphaMask();

private:
    char Unrecovered[0x48 - sizeof(ox::video::IImage)];
};

} // end namespace video
} // end namespace daisy

#endif
