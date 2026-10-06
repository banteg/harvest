// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/COpenGLTexture.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "COpenGLTexture.h"
#include "daisy/os.h"
#include "ox/IOxDevice.h"
#include "ox/video/IVideoDriver.h"
#include <GL/glu.h>
// The native object includes an iostream static initializer.
#include <iostream> // IWYU pragma: keep

#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

namespace daisy {
namespace video {

COpenGLTexture::COpenGLTexture(ox::video::IImage* image, bool generateMipLevels)
    : ImageSize(0, 0), OriginalSize(0, 0), Pitch(0), ImageData(0), TextureName(0), HasMipMaps(generateMipLevels)
{
    if (image)
    {
        getImageData(image);

        if (ImageData)
        {
            glGenTextures(1, &TextureName);
            copyTexture();
        }
    }
}

COpenGLTexture::~COpenGLTexture()
{
    delete[] ImageData;
    glDeleteTextures(1, &TextureName);
}

void COpenGLTexture::getImageData(ox::video::IImage* image)
{
    ImageSize = image->getDimension();
    OriginalSize = ImageSize;

    ox::core::CDimension2d<int> nImageSize;
    nImageSize.Width = getTextureSizeFromSurfaceSize(ImageSize.Width);
    nImageSize.Height = getTextureSizeFromSurfaceSize(ImageSize.Height);

    if (!nImageSize.Width || !nImageSize.Height || !ImageSize.Width || !ImageSize.Height)
    {
        os::Printer::log("Could not create OpenGL Texture.", ox::event::ELL_ERROR);
        return;
    }

    ImageData = new int[nImageSize.Width * nImageSize.Height];

    if (image->getColorFormat() == ox::video::ECF_A8R8G8B8)
    {
        // Original bug: the ARGB pixels are copied as one block of the texture's size, so an image
        // that is not a power of two in size is read past its end and its rows shear.
        int s = nImageSize.Width * nImageSize.Height;
        int* t = (int*)image->lock();
        for (int i = 0; i < s; ++i)
            ImageData[i] = t[i];
        image->unlock();
    }
    else
    {
        // other formats are converted pixel by pixel into the upper left corner
        for (int y = 0; y < ImageSize.Height; ++y)
            for (int x = 0; x < ImageSize.Width; ++x)
                ImageData[y * nImageSize.Width + x] = image->getPixel(x, y).color;
    }

    ImageSize = nImageSize;
}

//! Uploads the pixels with linear filtering; with mip maps (ETCF_CREATE_MIP_MAPS, which the game
//! switches off) the minification filter becomes GL_LINEAR_MIPMAP_NEAREST.
void COpenGLTexture::copyTexture()
{
    glBindTexture(GL_TEXTURE_2D, TextureName);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ImageSize.Width, ImageSize.Height, 0, GL_BGRA_EXT,
        GL_UNSIGNED_BYTE, ImageData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (HasMipMaps)
    {
        int ret = gluBuild2DMipmaps(GL_TEXTURE_2D, 4, ImageSize.Width, ImageSize.Height, GL_BGRA_EXT,
            GL_UNSIGNED_BYTE, ImageData);

        if (!ret)
        {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
    }
}

void* COpenGLTexture::lock()
{
    return ImageData;
}

void COpenGLTexture::unlock()
{
    copyTexture();
}

const ox::core::CDimension2d<int>& COpenGLTexture::getOriginalSize()
{
    return OriginalSize;
}

const ox::core::CDimension2d<int>& COpenGLTexture::getSize()
{
    return ImageSize;
}

int COpenGLTexture::getDriverType()
{
    return ox::video::EDT_OPENGL;
}

int COpenGLTexture::getColorFormat()
{
    return ox::video::ECF_A8R8G8B8;
}

int COpenGLTexture::getPitch()
{
    return ImageSize.Width * 4;
}

GLuint COpenGLTexture::getOpenGLTextureName()
{
    return TextureName;
}

} // end namespace video
} // end namespace daisy
