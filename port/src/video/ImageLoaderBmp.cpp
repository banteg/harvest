// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CImageLoaderBmp.cpp for the Harvest port; see
// third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
//
// The BMP loader CVideoNull creates first. The game's only BMP is the GUI's built-in font
// (BuildInFont.cpp: 128 x 128, 4 bits per pixel with a 16-colour palette, uncompressed), which
// CGUIEnvironment loads from memory as "#DefaultFont", so only the content test finds it.
//
// Irrlicht 0.7's rules: the name test is a case-sensitive ".bmp" anywhere in the name; the content
// test is the "BM" id; compressed files are refused (so its RLE decoders are unreachable and left
// out); 1, 4 and 8 bits per pixel become A1R5G5B5 through the palette, 24 bits R8G8B8; 16 and
// 32 bits give no image. Rows are stored bottom-up and the converters flip them. The header is read
// field by field, little-endian, instead of as a packed struct.

#include "daisy/os.h"
#include "daisy/video/Null/CColorConverter.h"
#include "daisy/video/Null/CImage.h"
#include "ox/io/IReadFile.h"
#include "ox/video/IImageLoader.h"
#include "video/ColorFormats.h"
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <vector>

namespace daisy {
namespace video {

namespace {

const int FILE_HEADER_SIZE = 14;
const int INFO_HEADER_SIZE = 40;

unsigned int readU32(const unsigned char* p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned int)p[3] << 24);
}

unsigned short readU16(const unsigned char* p)
{
    return (unsigned short)(p[0] | (p[1] << 8));
}

class CImageLoaderBmp : public ox::video::IImageLoader
{
public:
    virtual bool isALoadableFileExtension(const char* fileName)
    {
        return strstr(fileName, ".bmp") != 0;
    }

    virtual bool isALoadableFileFormat(ox::io::IReadFile* file)
    {
        unsigned char id[2];
        return file && file->read(id, 2) == 2 && readU16(id) == 0x4d42;
    }

    virtual ox::video::IImage* loadImage(ox::io::IReadFile* file)
    {
        unsigned char raw[FILE_HEADER_SIZE + INFO_HEADER_SIZE];
        if (!file || !file->seek(0) || file->read(raw, sizeof(raw)) != sizeof(raw))
            return 0;

        const unsigned int id = readU16(raw);
        const unsigned int declaredSize = readU32(raw + 2);
        const unsigned int offset = readU32(raw + 10);
        const unsigned int dibSize = readU32(raw + 14);
        const unsigned int width = readU32(raw + 18);
        const unsigned int height = readU32(raw + 22);
        const unsigned int bpp = readU16(raw + 28);
        const unsigned int dataSize = readU32(raw + 34);
        const unsigned int colors = readU32(raw + 46);
        if ((id != 0x4d42 && id != 0x424d) || readU16(raw + 26) != 1 ||
            (bpp != 1 && bpp != 4 && bpp != 8 && bpp != 24))
            return 0;

        if (readU32(raw + 30) != 0)
        {
            os::Printer::log("Compressed BMPs are currently not supported.", "", ox::event::ELL_ERROR);
            return 0;
        }

        const int actualSize = file->getSize();
        if (actualSize < (int)sizeof(raw) || declaredSize > (unsigned int)actualSize)
            return 0;
        const unsigned int fileSize = declaredSize ? declaredSize : (unsigned int)actualSize;
        if (offset < sizeof(raw) || offset > fileSize || dibSize < INFO_HEADER_SIZE ||
            dibSize > offset - FILE_HEADER_SIZE)
            return 0;

        // Only bottom-up images are supported. CImage and the converters count bytes in signed ints.
        if (!width || width > INT_MAX || !height || height > INT_MAX ||
            (uint64_t)width * height * (bpp == 24 ? 3 : 2) > INT_MAX)
            return 0;
        const unsigned int rowBytes = (unsigned int)(((uint64_t)width * bpp + 7) / 8);
        const unsigned int stride = (rowBytes + 3) & ~3u;
        const uint64_t required = (uint64_t)stride * height;
        if (required > fileSize - offset || (dataSize && (dataSize < required || dataSize > fileSize - offset)))
            return 0;

        const unsigned int paletteOffset = FILE_HEADER_SIZE + dibSize;
        const unsigned int maxColors = bpp < 24 ? 1u << bpp : 0;
        const unsigned int paletteSize = colors ? colors : maxColors;
        if (paletteSize > (offset - paletteOffset) / 4 || (maxColors && paletteSize > maxColors))
            return 0;
        int palette[256];
        if (maxColors)
        {
            unsigned char bytes[256 * 4];
            if (!file->seek(paletteOffset) || file->read(bytes, paletteSize * 4) != (int)paletteSize * 4)
                return 0;
            for (unsigned int i = 0; i < paletteSize; ++i)
                palette[i] = (int)readU32(bytes + i * 4);
        }

        std::vector<char> data((size_t)required);
        if (!file->seek(offset) || file->read(&data[0], (int)required) != (int)required)
            return 0;
        if (paletteSize < maxColors)
        {
            // A shortened palette is valid when every pixel (excluding padding bits and bytes) fits.
            const unsigned int perByte = 8 / bpp;
            for (unsigned int y = 0; y < height; ++y)
                for (unsigned int x = 0; x < width; ++x)
                {
                    const unsigned char packed = data[y * stride + x / perByte];
                    const unsigned int shift = (perByte - 1 - x % perByte) * bpp;
                    if (((packed >> shift) & (maxColors - 1)) >= paletteSize)
                        return 0;
                }
        }

        const ox::core::CDimension2d<int> size(width, height);
        const int pitch = stride - rowBytes;
        CImage* image = 0;
        switch (bpp)
        {
        case 1:
            image = new CImage(ox::video::ECF_A1R5G5B5, size);
            CColorConverter::convert1BitTo16BitFlipMirror(&data[0], (short*)image->lock(), width, height, pitch);
            image->unlock();
            break;
        case 4:
            image = new CImage(ox::video::ECF_A1R5G5B5, size);
            CColorConverter::convert4BitTo16BitFlipMirror(&data[0], (short*)image->lock(), width, height, pitch, palette);
            image->unlock();
            break;
        case 8:
            image = new CImage(ox::video::ECF_A1R5G5B5, size);
            CColorConverter::convert8BitTo16BitFlipMirror(&data[0], (short*)image->lock(), width, height, pitch, palette);
            image->unlock();
            break;
        case 24:
            image = new CImage(port::ECF_R8G8B8, size);
            CColorConverter::convert24BitTo24BitFlipMirrorColorShuffle(&data[0], (char*)image->lock(), width, height,
                pitch);
            image->unlock();
            break;
        }

        return image;
    }
};

} // end anonymous namespace

ox::video::IImageLoader* createImageLoaderBmp()
{
    return new CImageLoaderBmp();
}

} // end namespace video
} // end namespace daisy
