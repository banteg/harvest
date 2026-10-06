// The JPEG and TGA loaders CVideoNull creates, over stb_image, with the original's rules
// (docs/port/textures.md):
//
// - By name, a case-sensitive strstr for ".jpg" or ".tga" anywhere in the name.
// - By content, JPEG needs "JFIF" at offset 6 and TGA needs image type 2 (Irrlicht 0.7's tests).
// - JPEG decodes to R8G8B8.
// - TGA: only types 2 and 10 (uncompressed and RLE true-colour) at 16, 24 or 32 bits, as the
//   original; 32 bits give A8R8G8B8 words (0xAARRGGBB), 24 bits R8G8B8, 16 bits A1R5G5B5. Every
//   TGA is taken as bottom-up whatever its descriptor says, so a top-left-origin file loads upside
//   down, as in the original; the right-origin bit is ignored (stb_image ignores it too).
//
// Differences: grayscale JPEGs decode correctly (the original garbled them), and a 16-bit TGA
// always loads opaque (stb_image drops the attribute bit).

#include "daisy/os.h"
#include "daisy/video/Null/CImage.h"
#include "ox/io/IReadFile.h"
#include "ox/video/IImageLoader.h"
#include "video/ColorFormats.h"

#include <stb_image.h>
#include <string.h>
#include <vector>

namespace daisy {
namespace video {

namespace {

using ox::video::IImage;

//! The whole file, from the start.
bool readWholeFile(ox::io::IReadFile* file, std::vector<unsigned char>& data)
{
    int size = file->getSize();
    if (size <= 0)
        return false;
    data.resize(size);
    file->seek(0);
    return file->read(&data[0], size) == size;
}

class CImageLoaderJPG : public ox::video::IImageLoader
{
public:
    virtual bool isALoadableFileExtension(const char* fileName)
    {
        return strstr(fileName, ".jpg") != 0;
    }

    virtual bool isALoadableFileFormat(ox::io::IReadFile* file)
    {
        if (!file)
            return false;

        unsigned char jfif[4] = {0, 0, 0, 0};
        file->seek(6);
        file->read(jfif, 4);
        return memcmp(jfif, "JFIF", 4) == 0 || memcmp(jfif, "FIFJ", 4) == 0;
    }

    virtual IImage* loadImage(ox::io::IReadFile* file)
    {
        std::vector<unsigned char> data;
        if (!readWholeFile(file, data))
            return 0;

        int width, height, channels;
        unsigned char* pixels = stbi_load_from_memory(&data[0], (int)data.size(), &width, &height, &channels, 3);
        if (!pixels)
        {
            os::Printer::log("Could not decode JPEG file", file->getFileName(), ox::event::ELL_ERROR);
            return 0;
        }

        CImage* image = new CImage(port::ECF_R8G8B8, ox::core::CDimension2d<int>(width, height));
        memcpy(image->lock(), pixels, width * height * 3);
        image->unlock();
        stbi_image_free(pixels);
        return image;
    }
};

class CImageLoaderTGA : public ox::video::IImageLoader
{
public:
    virtual bool isALoadableFileExtension(const char* fileName)
    {
        return strstr(fileName, ".tga") != 0;
    }

    virtual bool isALoadableFileFormat(ox::io::IReadFile* file)
    {
        if (!file)
            return false;

        unsigned char type[3] = {0, 0, 0};
        file->read(type, 3);
        return type[2] == 2;
    }

    virtual IImage* loadImage(ox::io::IReadFile* file)
    {
        std::vector<unsigned char> data;
        if (!readWholeFile(file, data) || data.size() < 18)
            return 0;

        const unsigned char imageType = data[2];
        const unsigned char pixelDepth = data[16];
        const bool topOrigin = (data[17] & 0x20) != 0;

        if (imageType != 2 && imageType != 10)
        {
            os::Printer::log("Unsupported TGA file type", file->getFileName(), ox::event::ELL_ERROR);
            return 0;
        }
        if (pixelDepth != 16 && pixelDepth != 24 && pixelDepth != 32)
        {
            if (pixelDepth == 8)
                os::Printer::log("Unsupported TGA format, 8 bit", file->getFileName(), ox::event::ELL_ERROR);
            return 0;
        }

        const int channels = pixelDepth == 32 ? 4 : 3;
        int width, height, fileChannels;
        unsigned char* pixels =
            stbi_load_from_memory(&data[0], (int)data.size(), &width, &height, &fileChannels, channels);
        if (!pixels)
        {
            os::Printer::log("Could not decode TGA file", file->getFileName(), ox::event::ELL_ERROR);
            return 0;
        }

        ox::video::ECOLOR_FORMAT format = pixelDepth == 32 ? ox::video::ECF_A8R8G8B8
            : pixelDepth == 24                             ? port::ECF_R8G8B8
                                                           : ox::video::ECF_A1R5G5B5;
        CImage* image = new CImage(format, ox::core::CDimension2d<int>(width, height));
        unsigned char* out = (unsigned char*)image->lock();
        const int outPitch = width * image->getBytesPerPixel();

        for (int y = 0; y < height; ++y)
        {
            // stb_image returns rows top-down for either origin; the original always flips the
            // rows as stored, so a file stored top-down comes out upside down.
            const unsigned char* in = pixels + (topOrigin ? height - 1 - y : y) * width * channels;
            unsigned char* row = out + y * outPitch;
            for (int x = 0; x < width; ++x, in += channels)
            {
                if (pixelDepth == 32)
                    ((unsigned int*)row)[x] = (unsigned int)in[3] << 24 | in[0] << 16 | in[1] << 8 | in[2];
                else if (pixelDepth == 24)
                    memcpy(row + x * 3, in, 3);
                else
                    ((unsigned short*)row)[x] =
                        (unsigned short)(0x8000 | (in[0] >> 3) << 10 | (in[1] >> 3) << 5 | in[2] >> 3);
            }
        }

        image->unlock();
        stbi_image_free(pixels);
        return image;
    }
};

} // end anonymous namespace

ox::video::IImageLoader* createImageLoaderJPG()
{
    return new CImageLoaderJPG();
}

ox::video::IImageLoader* createImageLoaderTGA()
{
    return new CImageLoaderTGA();
}

} // end namespace video
} // end namespace daisy
