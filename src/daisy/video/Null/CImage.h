// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Irrlicht 0.7's CImage (third_party/irrlicht-0.7/source/Irrlicht/CImage.h). Partial: the copy,
// drawing and scaling methods are not declared. The members are Irrlicht's, at the offsets the
// Linux accessors read (Data 0x18, Size 0x20, BitsPerPixel 0x28, BytesPerPixel 0x2c, Format 0x30,
// masks 0x34..0x40; 0x48 bytes). Only the port declares them: in the matching build they change
// CVideoNull's code, so it keeps the sized placeholder.

#ifndef DAISY_VIDEO_NULL_CIMAGE_H
#define DAISY_VIDEO_NULL_CIMAGE_H

#include "ox/video/IImage.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CRect.h"
#include "ox/video/SColor.h"
#include "ox/video/IVideoDriver.h"

namespace ox { namespace video {} }
namespace daisy {
namespace video {
using namespace ox::video;

//! ox::core::A software image.
#ifdef HARVEST_PORT
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
#else
class CImage : public IImage
{
public:

    //! constructor
    CImage(ECOLOR_FORMAT format, IImage* imageToCopy);

    //! constructor
    CImage(ECOLOR_FORMAT format, const ox::core::CDimension2d<int>& size,
        void* data);

    //! constructor
    CImage(ECOLOR_FORMAT format, const ox::core::CDimension2d<int>& size);

    //! constructor
    CImage(IImage* imageToCopy,
        const ox::core::CPosition2d<int>& pos, const ox::core::CDimension2d<int>& size);

    //! destructor
    virtual ~CImage();

    //! Lock function.
    virtual void* lock();

    //! Unlock function.
    virtual void unlock();

    //! Returns width and height of image data.
    virtual const ox::core::CDimension2d<int>& getDimension();

    //! Returns bits per pixel.
    virtual int getBitsPerPixel();

    //! Returns bytes per pixel
    virtual int getBytesPerPixel();

    //! Returns image data size in bytes
    virtual int getImageDataSizeInBytes();

    //! Returns image data size in pixels
    virtual int getImageDataSizeInPixels();

    //! returns mask for red value of a pixel
    virtual unsigned int getRedMask();

    //! returns mask for green value of a pixel
    virtual unsigned int getGreenMask();

    //! returns mask for blue value of a pixel
    virtual unsigned int getBlueMask();

    //! returns mask for alpha value of a pixel
    virtual unsigned int getAlphaMask();

    //! returns a pixel
    virtual SColor getPixel(int x, int y);

    //! returns the color format
    virtual int getColorFormat();

    //! draws a rectangle
    void drawRectangle(const ox::core::CRect<int>& rect, SColor color);

    //! draws a rectangle
    void drawRectangle(int x, int y, int x2, int y2, SColor color);

    //! copies this surface into another
    void copyTo(CImage* target, int x, int y);

    //! copies this surface into another
    void copyTo(CImage* target, const ox::core::CPosition2d<int>& pos);

    //! copies this surface into another
    void copyTo(CImage* target, const ox::core::CPosition2d<int>& pos, const ox::core::CRect<int>& sourceRect, const ox::core::CRect<int>* clipRect=0);

    //! copies this surface into another, using the alpha mask
    void copyToWithAlpha(CImage* target, const ox::core::CPosition2d<int>& pos, const ox::core::CRect<int>& sourceRect);

    //! copies this surface into another, using the alpha mask, an cliprect and a color to add with
    void copyToWithAlpha(CImage* target, const ox::core::CPosition2d<int>& pos, const ox::core::CRect<int>& sourceRect, short color, const ox::core::CRect<int>* clipRect = 0);

    //! copies this surface into another, scaling it to fit it.
    void copyToScaling(CImage* target);

    //! draws a line from to
    void drawLine(const ox::core::CPosition2d<int>& from, const ox::core::CPosition2d<int>& to, short color);

    //! resizes the surface to a new size
    void resizeTo(const ox::core::CDimension2d<int>& size);

    //! fills the surface with black or white
    void fill(short color);

private:

    //! assumes format and size has been set and creates the rest
    void initData();

    //! clips a x coordinate into the screen
    inline void clipX(int &x);

    //! clips a y coordinate into the screen
    inline void clipY(int &y);

    //! exchanges two ints
    inline void exchange(int& a, int& b);

    //! clips coordinates into the rect. returns false if the rects is completely outside
    //! and does not need to be drawn.
    inline bool clipRect(const int targetWidth, const int targetHeight, int& targetX, int& targetY, int& xInSource, int& yInSource, int& sourceWidth, int& soureHeight);

    //! sets a pixel very fast and inline
    inline void setPixelFast(int x, int y, short color);

    int getBitsPerPixelFromFormat();

    void* Data;
    ox::core::CDimension2d<int> Size;
    int BitsPerPixel;
    int BytesPerPixel;
    ECOLOR_FORMAT Format;

    int RedMask;
    int GreenMask;
    int BlueMask;
    int AlphaMask;
};
#endif

} // end namespace video
} // end namespace daisy

#endif
