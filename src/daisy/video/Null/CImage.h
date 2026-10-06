// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Irrlicht 0.7's CImage (third_party/irrlicht-0.7/source/Irrlicht/CImage.h). Partial: the copy,
// drawing and scaling methods are not declared. The members are Irrlicht's, at the offsets the
// Linux accessors read (Data 0x18, Size 0x20, BitsPerPixel 0x28, BytesPerPixel 0x2c, Format 0x30,
// masks 0x34..0x40; 0x48 bytes). Only the port declares them: in the matching build they change
// CVideoNull's code, so it keeps the sized placeholder.

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
#ifdef HARVEST_PORT
    //! Sets the bits and bytes per pixel and allocates Data unless it is already set.
    void initData();
    //! Sets the channel masks for Format and returns its bits per pixel (0 if unknown).
    int getBitsPerPixelFromFormat();

    void* Data;
    ox::core::CDimension2d<int> Size;
    int BitsPerPixel;
    int BytesPerPixel;
    ox::video::ECOLOR_FORMAT Format;

    unsigned int RedMask;
    unsigned int GreenMask;
    unsigned int BlueMask;
    unsigned int AlphaMask;
#else
    char Unrecovered[0x48 - sizeof(ox::video::IImage)];
#endif
};

} // end namespace video
} // end namespace daisy

#endif
