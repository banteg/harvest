// The renderer's texture: the original's COpenGLTexture (src/daisy/video/OpenGL/COpenGLTexture.*)
// on OpenGL ES 3.0 / OpenGL 3.3 core.
//
// Differences from the original, all invisible for the game's power-of-two textures:
// - The texture has the image's size (OpenGL ES 3.0 and OpenGL 3.3 take any size), instead of the
//   next power of two with the image in the upper left corner. getSize therefore equals
//   getOriginalSize, and 2D texture coordinates, which the driver divides by the original size, and
//   the sprite package's 3D coordinates, divided by getSize, address exactly the image.
// - A8R8G8B8 images are copied at their own size, so the original's read past the end of a
//   non-power-of-two image (original-bugs.md) cannot happen.
// - The pixels are kept as A8R8G8B8 like the original's, but uploaded as RGBA bytes: OpenGL ES has no
//   GL_BGRA upload.
// - Uploads put back the texture binding they change, so the driver's cache of bound textures stays
//   right after unlock (the original rebinds the active unit behind the cache's back).

#ifndef PORT_VIDEO_CTEXTUREGL_H
#define PORT_VIDEO_CTEXTUREGL_H

#include "video/GL.h"
#include "ox/video/IImage.h"
#include "ox/video/ITexture.h"

namespace port {
namespace video {

class CTextureGL : public ox::video::ITexture
{
public:
    //! image may be 0 for an empty texture without an OpenGL name (createScreenTexture).
    //! generateMipLevels builds mip maps (ETCF_CREATE_MIP_MAPS).
    CTextureGL(ox::video::IImage* image, bool generateMipLevels);
    virtual ~CTextureGL();

    //! The A8R8G8B8 pixels, getPitch bytes per row; unlock uploads them again.
    virtual void* lock();
    virtual void unlock();
    virtual const ox::core::CDimension2d<int>& getOriginalSize();
    virtual const ox::core::CDimension2d<int>& getSize();
    //! EDT_OPENGL, which the driver's setTexture checks.
    virtual int getDriverType();
    //! Always A8R8G8B8 (the font code reads this to choose 32-bit parsing).
    virtual int getColorFormat();
    virtual int getPitch();

    gl::GLuint getTextureName() const { return TextureName; }

private:
    void getImageData(ox::video::IImage* image);
    //! Uploads ImageData with linear filters, and mip maps if requested.
    void copyTexture();

    ox::core::CDimension2d<int> Size;
    unsigned int* ImageData;
    gl::GLuint TextureName;
    bool HasMipMaps;
};

} // end namespace video
} // end namespace port

#endif
