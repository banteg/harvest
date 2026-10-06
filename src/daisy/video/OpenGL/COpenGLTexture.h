// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/COpenGLTexture.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef DAISY_VIDEO_OPENGL_COPENGLTEXTURE_H
#define DAISY_VIDEO_OPENGL_COPENGLTEXTURE_H

#include "ox/video/IImage.h"
#include "ox/video/ITexture.h"

#define GL_GLEXT_LEGACY 1
#include <GL/gl.h>
#undef GL_GLEXT_LEGACY

namespace daisy {
namespace video {

//! OpenGL texture. The pixels are kept in system memory as 32 bit ARGB (ECF_A8R8G8B8, uploaded as
//! GL_BGRA bytes into a GL_RGBA texture) at the next power of two size; the image sits in the upper
//! left corner and the rest is undefined (see getImageData).
class COpenGLTexture : public ox::video::ITexture
{
public:
    //! surface may be 0 for an empty texture without OpenGL name.
    COpenGLTexture(ox::video::IImage* surface, bool generateMipLevels);

    virtual ~COpenGLTexture();

    //! Returns the ARGB pixels; unlock uploads them again.
    virtual void* lock();
    virtual void unlock();
    //! The size of the image the texture was made from.
    virtual const ox::core::CDimension2d<int>& getOriginalSize();
    //! The power of two size of the OpenGL texture.
    virtual const ox::core::CDimension2d<int>& getSize();
    virtual int getDriverType();
    virtual int getColorFormat();
    virtual int getPitch();

    GLuint getOpenGLTextureName();

private:
    //! Copies the image into ImageData at the power of two size.
    void getImageData(ox::video::IImage* image);

    //! Uploads ImageData into the OpenGL texture.
    void copyTexture();

    //! The next power of two of size.
    inline int getTextureSizeFromSurfaceSize(int size)
    {
        int ts = 0x01;
        while (ts < size)
            ts <<= 1;

        return ts;
    }

    // 0x18
    ox::core::CDimension2d<int> ImageSize;
    ox::core::CDimension2d<int> OriginalSize;
    int Pitch;
    bool SurfaceHasSameSize;
    // 0x30
    int* ImageData;
    GLuint TextureName;
    bool HasMipMaps;
};

} // end namespace video
} // end namespace daisy

#endif
