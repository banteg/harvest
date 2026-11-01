// Standalone image loader checks: zig build test-image_loaders.
#include "daisy/gui/BuildInFont.h"
#include "ox/io/CMemReadFile.h"
#include "ox/video/IImageLoader.h"
#include "ox/video/IImage.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>
#include <stb_image.h>
#include <stb_image_write.h>

namespace daisy { namespace video {
ox::video::IImageLoader* createImageLoaderBmp();
ox::video::IImageLoader* createImageLoaderPSD();
ox::video::IImageLoader* createImageLoaderJPG();
ox::video::IImageLoader* createImageLoaderTGA();
} }

// Make missing channel initialization visible regardless of the allocator's previous contents.
void* operator new[](std::size_t size) throw(std::bad_alloc)
{
    void* memory = std::malloc(size);
    if (!memory)
        throw std::bad_alloc();
    std::memset(memory, 0x5a, size);
    return memory;
}
void operator delete[](void* memory) throw() { std::free(memory); }

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "Image loader check failed: %s\n", #c); return 1; } } while (0)
typedef std::vector<unsigned char> Bytes;

static void word(Bytes& data, unsigned int value)
{
    data.push_back((unsigned char)(value >> 8));
    data.push_back((unsigned char)value);
}
static void dword(Bytes& data, unsigned int value)
{
    word(data, value >> 16);
    word(data, value);
}
static Bytes header(int width, int height, int channels, int compression)
{
    Bytes data;
    const char* signature = "8BPS";
    data.insert(data.end(), signature, signature + 4);
    word(data, 1);
    data.insert(data.end(), 6, 0);
    word(data, channels);
    dword(data, height);
    dword(data, width);
    word(data, 8);
    word(data, 3);
    for (int i = 0; i < 3; ++i)
        dword(data, 0); // No color-mode data, image resources or layer/mask data.
    word(data, compression);
    return data;
}
static ox::video::IImage* load(ox::video::IImageLoader* loader, Bytes& data)
{
    ox::io::CMemReadFile file(&data[0], (int)data.size(), false);
    return loader->loadImage(&file);
}
static void encoded(void* context, void* data, int size)
{
    Bytes& result = *(Bytes*)context;
    unsigned char* bytes = (unsigned char*)data;
    result.insert(result.end(), bytes, bytes + size);
}

static void littleDword(Bytes& data, int at, unsigned int value)
{
    for (int i = 0; i < 4; ++i)
        data[at + i] = (unsigned char)(value >> (8 * i));
}

class ShortBmpFile : public ox::io::CMemReadFile
{
public:
    ShortBmpFile(Bytes& bytes, int stop) : CMemReadFile(&bytes[0], (int)bytes.size(), false), Stop(stop) {}
    int read(void* data, int count) { return CMemReadFile::read(data, getPos() == Stop ? count - 1 : count); }
private:
    int Stop;
};

static int checkBmp()
{
    ox::video::IImageLoader* bmp = daisy::video::createImageLoaderBmp();
    Bytes rgb(62, 0); // Two bottom-up padded 24-bit rows, one pixel each.
    rgb[0] = 'B'; rgb[1] = 'M'; rgb[2] = 62; rgb[10] = 54; rgb[14] = 40;
    rgb[18] = 1; rgb[22] = 2; rgb[26] = 1; rgb[28] = 24;
    rgb[54] = 255; rgb[60] = 255;
    ox::video::IImage* image = load(bmp, rgb);
    CHECK(image && (unsigned int)image->getPixel(0, 0).color == 0xffff0000u);
    CHECK((unsigned int)image->getPixel(0, 1).color == 0xff0000ffu);
    image->drop();

    Bytes invalid = rgb; invalid[18] = 0;
    CHECK(!load(bmp, invalid));
    invalid = rgb; littleDword(invalid, 22, 0xffffffffu); // Unsupported top-down image.
    CHECK(!load(bmp, invalid));
    invalid = rgb; littleDword(invalid, 22, 0x40000000u); // CImage output size overflows.
    CHECK(!load(bmp, invalid));
    invalid = rgb; invalid[34] = 4; // biSizeImage is shorter than both padded rows.
    CHECK(!load(bmp, invalid));
    invalid = rgb; invalid[2] = 0; invalid.pop_back(); // Truncated file, both size fields omitted.
    CHECK(!load(bmp, invalid));
    invalid = rgb; littleDword(invalid, 34, 0xfffffffdu);
    CHECK(!load(bmp, invalid));
    invalid = rgb; invalid[10] = 20;
    CHECK(!load(bmp, invalid));
    invalid = rgb; littleDword(invalid, 10, 0xfffffffcu);
    CHECK(!load(bmp, invalid));
    ShortBmpFile shortPixels(rgb, 54);
    CHECK(!bmp->loadImage(&shortPixels));

    // A two-entry palette, followed by a row of three pixels and unused padding bits/bytes.
    Bytes indexed = rgb;
    indexed.resize(66, 0); indexed[2] = 66; indexed[10] = 62; indexed[18] = 3; indexed[22] = 1;
    indexed[28] = 4; indexed[46] = 2; indexed[54] = 0; indexed[60] = 255;
    indexed[62] = 0x10; indexed[63] = 0x1f; indexed[64] = 0xff;
    image = load(bmp, indexed);
    CHECK(image && (unsigned int)image->getPixel(0, 0).color == 0xfff80000u);
    CHECK((unsigned int)image->getPixel(1, 0).color == 0xff000000u);
    CHECK((unsigned int)image->getPixel(2, 0).color == 0xfff80000u);
    image->drop();
    Bytes eight = indexed; eight[28] = 8; eight[62] = 1; eight[63] = 0; eight[64] = 1;
    image = load(bmp, eight);
    CHECK(image && (unsigned int)image->getPixel(2, 0).color == 0xfff80000u);
    image->drop();
    Bytes one = indexed; one[28] = 1; one[62] = 0xa0;
    image = load(bmp, one);
    CHECK(image && (unsigned int)image->getPixel(2, 0).color == 0xfff8f8f8u);
    CHECK(image->getPixel(1, 0).color == 0);
    image->drop();
    invalid = indexed; invalid[62] = 0x12;
    CHECK(!load(bmp, invalid)); // The second nibble names a missing palette entry.
    invalid = indexed; invalid[28] = 8; invalid[62] = 255;
    CHECK(!load(bmp, invalid));
    invalid = indexed; invalid[46] = 0;
    CHECK(!load(bmp, invalid)); // No explicit color count means the full palette is required.
    invalid = indexed; invalid[14] = 44;
    CHECK(!load(bmp, invalid)); // The extended DIB header overlaps the claimed palette.
    ShortBmpFile shortPalette(indexed, 54);
    CHECK(!bmp->loadImage(&shortPalette));

    ox::io::CMemReadFile font(daisy::gui::BuildInFontData, daisy::gui::BuildInFontDataSize, false);
    CHECK(bmp->isALoadableFileFormat(&font));
    image = bmp->loadImage(&font);
    CHECK(image && image->getDimension().Width == 128 && image->getDimension().Height == 128);
    const unsigned short* pixels = (const unsigned short*)image->lock();
    unsigned int hash = 2166136261u;
    for (int i = 0; i < 128 * 128; ++i)
        hash = (hash ^ pixels[i]) * 16777619u;
    image->unlock();
    CHECK(hash == 0xa5ba448au);
    image->drop();
    bmp->drop();
    return 0;
}

int main()
{
    CHECK(checkBmp() == 0);
    ox::video::IImageLoader* loader = daisy::video::createImageLoaderPSD();
    CHECK(loader != 0);
    CHECK(loader->isALoadableFileExtension("image.psd.backup"));
    CHECK(!loader->isALoadableFileExtension("image.PSD"));
    CHECK(!loader->isALoadableFileFormat(0));
    Bytes raw = header(2, 2, 3, 0);
    unsigned char planes[] = {255, 0, 128, 128, 0, 0, 64, 32, 10, 20, 0, 0};
    raw.insert(raw.end(), planes, planes + sizeof(planes));
    ox::io::CMemReadFile file(&raw[0], (int)raw.size(), false);
    CHECK(loader->isALoadableFileFormat(&file));
    unsigned int expected[] = {0xffff000a, 0xff000014, 0xff804000, 0xff802000};
    ox::video::IImage* image = loader->loadImage(&file);
    CHECK(image != 0 && image->getDimension().Width == 2 && image->getDimension().Height == 2);
    for (int i = 0; i < 4; ++i)
        CHECK((unsigned int)image->getPixel(i % 2, i / 2).color == expected[i]);
    CHECK(image->drop());

    Bytes rle = header(2, 2, 3, 1);
    unsigned short counts[] = {3, 2, 2, 3, 4, 2};
    for (int i = 0; i < 6; ++i)
        word(rle, counts[i]);
    unsigned char rows[] = {1, 255, 0, 255, 128, 255, 0, 1, 64, 32, 128, 1, 10, 20, 255, 0};
    rle.insert(rle.end(), rows, rows + sizeof(rows));
    image = load(loader, rle);
    CHECK(image != 0);
    for (int i = 0; i < 4; ++i)
        CHECK((unsigned int)image->getPixel(i % 2, i / 2).color == expected[i]);
    CHECK(image->drop());

    // Largest PackBits literal and repeat runs, plus a no-op control byte.
    Bytes wide = header(128, 1, 3, 1);
    word(wide, 129); word(wide, 2); word(wide, 130);
    wide.push_back(127);
    for (int i = 0; i < 128; ++i) wide.push_back((unsigned char)i);
    wide.push_back(129); wide.push_back(42);
    wide.push_back(128); wide.push_back(127);
    for (int i = 0; i < 128; ++i) wide.push_back((unsigned char)(i / 2));
    image = load(loader, wide);
    CHECK(image != 0 && image->getDimension().Width == 128);
    for (int i = 0; i < 128; ++i)
        CHECK((unsigned int)image->getPixel(i, 0).color == (0xff000000u | i << 16 | 42 << 8 | i / 2));
    CHECK(image->drop());

    Bytes alpha = header(2, 1, 4, 0);
    unsigned char rgba[] = {255, 0, 0, 255, 10, 20, 255, 0};
    alpha.insert(alpha.end(), rgba, rgba + sizeof(rgba));
    image = load(loader, alpha);
    CHECK(image != 0);
    CHECK((unsigned int)image->getPixel(0, 0).color == 0xffff000a);
    CHECK((unsigned int)image->getPixel(1, 0).color == 0x0000ff14);
    CHECK(image->drop());

    Bytes invalid = raw;
    invalid[5] = 2;
    CHECK(load(loader, invalid) == 0); // Unsupported version.
    invalid = raw; invalid[23] = 1;
    CHECK(load(loader, invalid) == 0); // Unsupported depth.
    invalid = raw; invalid[39] = 2;
    CHECK(load(loader, invalid) == 0); // Unsupported ZIP compression.
    invalid = raw; invalid.resize(4);
    CHECK(load(loader, invalid) == 0); // Truncated header.
    invalid = raw; invalid[0] = 'X';
    CHECK(load(loader, invalid) == 0);
    image = load(loader, raw);
    CHECK(image != 0 && (unsigned int)image->getPixel(0, 0).color == expected[0]);
    CHECK(image->drop());
    CHECK(loader->drop());

    // Enabling PSD must keep the port's existing JPEG/TGA/PNG codecs available.
    loader = daisy::video::createImageLoaderTGA();
    Bytes tga(18, 0);
    tga[2] = 2; tga[12] = 2; tga[14] = 1; tga[16] = 32;
    unsigned char bgra[] = {10, 0, 255, 255, 20, 255, 0, 255};
    tga.insert(tga.end(), bgra, bgra + sizeof(bgra));
    image = load(loader, tga);
    CHECK(image != 0 && (unsigned int)image->getPixel(0, 0).color == 0xffff000a);
    CHECK((unsigned int)image->getPixel(1, 0).color == 0xff00ff14);
    CHECK(image->drop()); CHECK(loader->drop());
    unsigned char rgb[] = {80, 120, 160, 80, 120, 160};
    Bytes jpg;
    CHECK(stbi_write_jpg_to_func(encoded, &jpg, 2, 1, 3, rgb, 95));
    loader = daisy::video::createImageLoaderJPG();
    image = load(loader, jpg);
    CHECK(image != 0 && image->getDimension().Width == 2);
    ox::video::SColor pixel = image->getPixel(0, 0);
    CHECK(std::abs(pixel.getRed() - 80) <= 4 && std::abs(pixel.getGreen() - 120) <= 4
        && std::abs(pixel.getBlue() - 160) <= 4 && pixel.getAlpha() == 255);
    CHECK(image->drop()); CHECK(loader->drop());
    Bytes png;
    CHECK(stbi_write_png_to_func(encoded, &png, 2, 1, 3, rgb, 6));
    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(&png[0], (int)png.size(), &width, &height, &channels, 4);
    CHECK(pixels != 0 && width == 2 && height == 1);
    for (int i = 0; i < 2; ++i)
        CHECK(pixels[i * 4] == 80 && pixels[i * 4 + 1] == 120 && pixels[i * 4 + 2] == 160
            && pixels[i * 4 + 3] == 255);
    stbi_image_free(pixels);
    std::puts("Image loader smoke passed: BMP bounds, palettes and font, raw/RLE PSD, alpha, bad headers, reuse and JPEG/TGA/PNG");
    return 0;
}
