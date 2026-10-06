#include "video/CTextureGL.h"
#include "daisy/os.h"
#include "ox/IOxDevice.h"
#include "ox/video/IVideoDriver.h"
#include <vector>

namespace port {
namespace video {

using namespace gl;

CTextureGL::CTextureGL(ox::video::IImage* image, bool generateMipLevels)
    : Size(0, 0), ImageData(0), TextureName(0), HasMipMaps(generateMipLevels)
{
    if (!image)
        return;

    getImageData(image);
    if (ImageData)
    {
        glGenTextures(1, &TextureName);
        copyTexture();
    }
}

CTextureGL::~CTextureGL()
{
    delete[] ImageData;
    if (TextureName)
        glDeleteTextures(1, &TextureName);
}

//! The original's COpenGLTexture::getImageData without the padding: A8R8G8B8 images are copied as
//! they are, other formats pixel by pixel through IImage::getPixel (R8G8B8 becomes opaque, A1R5G5B5
//! keeps its one alpha bit, and the R5G6B5 the main menu's AtrumBlack uses reads as 0).
void CTextureGL::getImageData(ox::video::IImage* image)
{
    Size = image->getDimension();
    if (Size.Width <= 0 || Size.Height <= 0)
    {
        daisy::os::Printer::log("Could not create OpenGL Texture.", ox::event::ELL_ERROR);
        return;
    }

    const int count = Size.Width * Size.Height;
    ImageData = new unsigned int[count];

    if (image->getColorFormat() == ox::video::ECF_A8R8G8B8)
    {
        const unsigned int* source = (const unsigned int*)image->lock();
        for (int i = 0; i < count; ++i)
            ImageData[i] = source[i];
        image->unlock();
    }
    else
    {
        for (int y = 0; y < Size.Height; ++y)
            for (int x = 0; x < Size.Width; ++x)
                ImageData[y * Size.Width + x] = image->getPixel(x, y).color;
    }
}

void CTextureGL::copyTexture()
{
    const int count = Size.Width * Size.Height;
    std::vector<unsigned char> rgba(count * 4);
    for (int i = 0; i < count; ++i)
    {
        const unsigned int argb = ImageData[i];
        rgba[i * 4 + 0] = (unsigned char)(argb >> 16);
        rgba[i * 4 + 1] = (unsigned char)(argb >> 8);
        rgba[i * 4 + 2] = (unsigned char)argb;
        rgba[i * 4 + 3] = (unsigned char)(argb >> 24);
    }

    GLint previous = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous);
    glBindTexture(GL_TEXTURE_2D, TextureName);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Size.Width, Size.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &rgba[0]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (HasMipMaps)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    glBindTexture(GL_TEXTURE_2D, (GLuint)previous);
}

void* CTextureGL::lock()
{
    return ImageData;
}

void CTextureGL::unlock()
{
    if (TextureName)
        copyTexture();
}

const ox::core::CDimension2d<int>& CTextureGL::getOriginalSize()
{
    return Size;
}

const ox::core::CDimension2d<int>& CTextureGL::getSize()
{
    return Size;
}

int CTextureGL::getDriverType()
{
    return ox::video::EDT_OPENGL;
}

int CTextureGL::getColorFormat()
{
    return ox::video::ECF_A8R8G8B8;
}

int CTextureGL::getPitch()
{
    return Size.Width * 4;
}

} // end namespace video
} // end namespace port
