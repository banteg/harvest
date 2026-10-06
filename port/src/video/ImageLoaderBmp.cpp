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
#include <string.h>
#include <vector>

namespace daisy {
namespace video {

namespace {

//! The file header and BITMAPINFOHEADER: 54 bytes.
struct SBMPHeader
{
    unsigned short Id;
    unsigned int FileSize;
    unsigned int Reserved;
    unsigned int BitmapDataOffset;
    unsigned int BitmapHeaderSize;
    unsigned int Width;
    unsigned int Height;
    unsigned short Planes;
    unsigned short BPP;
    unsigned int Compression;
    unsigned int BitmapDataSize;
    unsigned int PixelPerMeterX;
    unsigned int PixelPerMeterY;
    unsigned int Colors;
    unsigned int ImportantColors;
};

const int HEADER_SIZE = 54;

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
        unsigned char id[2] = {0, 0};
        file->read(id, 2);
        return readU16(id) == 0x4d42;
    }

    virtual ox::video::IImage* loadImage(ox::io::IReadFile* file)
    {
        unsigned char raw[HEADER_SIZE];
        file->seek(0);
        if (file->read(raw, HEADER_SIZE) != HEADER_SIZE)
            return 0;

        SBMPHeader header;
        header.Id = readU16(raw);
        header.FileSize = readU32(raw + 2);
        header.Reserved = readU32(raw + 6);
        header.BitmapDataOffset = readU32(raw + 10);
        header.BitmapHeaderSize = readU32(raw + 14);
        header.Width = readU32(raw + 18);
        header.Height = readU32(raw + 22);
        header.Planes = readU16(raw + 26);
        header.BPP = readU16(raw + 28);
        header.Compression = readU32(raw + 30);
        header.BitmapDataSize = readU32(raw + 34);
        header.PixelPerMeterX = readU32(raw + 38);
        header.PixelPerMeterY = readU32(raw + 42);
        header.Colors = readU32(raw + 46);
        header.ImportantColors = readU32(raw + 50);

        if (header.Id != 0x4d42 && header.Id != 0x424d)
            return 0;

        if (header.Compression != 0)
        {
            os::Printer::log("Compressed BMPs are currently not supported.", "", ox::event::ELL_ERROR);
            return 0;
        }

        // adjust the bitmap data size to a dword boundary
        header.BitmapDataSize += (4 - (header.BitmapDataSize % 4)) % 4;

        // the palette fills the space between the headers and the pixels
        const int pos = file->getPos();
        const int paletteSize = ((int)header.BitmapDataOffset - pos) / 4;
        std::vector<int> palette(paletteSize > 0 ? paletteSize : 1, 0);
        if (paletteSize > 0)
        {
            std::vector<unsigned char> bytes(paletteSize * 4);
            file->read(&bytes[0], paletteSize * 4);
            for (int i = 0; i < paletteSize; ++i)
                palette[i] = (int)readU32(&bytes[i * 4]);
        }

        // some writers leave the data size out
        if (!header.BitmapDataSize)
            header.BitmapDataSize = file->getSize() - header.BitmapDataOffset;

        file->seek(header.BitmapDataOffset);

        float t = header.Width * (header.BPP / 8.0f);
        int widthInBytes = (int)t;
        t -= widthInBytes;
        if (t != 0.0f)
            ++widthInBytes;
        const int lineData = widthInBytes + ((4 - (widthInBytes % 4))) % 4;
        const int pitch = lineData - widthInBytes;

        std::vector<char> data(header.BitmapDataSize > 0 ? header.BitmapDataSize : 1, 0);
        file->read(&data[0], header.BitmapDataSize);

        const ox::core::CDimension2d<int> size(header.Width, header.Height);
        CImage* image = 0;
        switch (header.BPP)
        {
        case 1:
            image = new CImage(ox::video::ECF_A1R5G5B5, size);
            CColorConverter::convert1BitTo16BitFlipMirror(&data[0], (short*)image->lock(), header.Width, header.Height,
                pitch);
            image->unlock();
            break;
        case 4:
            image = new CImage(ox::video::ECF_A1R5G5B5, size);
            CColorConverter::convert4BitTo16BitFlipMirror(&data[0], (short*)image->lock(), header.Width, header.Height,
                pitch, &palette[0]);
            image->unlock();
            break;
        case 8:
            image = new CImage(ox::video::ECF_A1R5G5B5, size);
            CColorConverter::convert8BitTo16BitFlipMirror(&data[0], (short*)image->lock(), header.Width, header.Height,
                pitch, &palette[0]);
            image->unlock();
            break;
        case 24:
            image = new CImage(port::ECF_R8G8B8, size);
            CColorConverter::convert24BitTo24BitFlipMirrorColorShuffle(&data[0], (char*)image->lock(), header.Width,
                header.Height, pitch);
            image->unlock();
            break;
        default:
            // 16 and 32 bits: no image, as in Irrlicht 0.7
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
