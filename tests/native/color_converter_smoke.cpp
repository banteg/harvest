#include "daisy/video/Null/CColorConverter.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

typedef daisy::video::CColorConverter Converter;
typedef ox::video::ECOLOR_FORMAT Format;

static void require(bool condition, const char* expression)
{
    if (!condition)
    {
        std::fprintf(stderr, "Color-converter check failed: %s\n", expression);
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expression) require((expression), #expression)

static unsigned short rgb(unsigned int r, unsigned int g, unsigned int b)
{
    return 0x8000u | ((r & 0xf8u) << 7) | ((g & 0xf8u) << 2) | (b >> 3);
}

static unsigned short paletteColor(unsigned int color)
{
    return rgb((color >> 16) & 255, (color >> 8) & 255, color & 255);
}

static void indexed_images()
{
    int palette[256];
    for (int i = 0; i < 256; ++i)
        palette[i] = (i << 16) | ((255 - i) << 8) | ((i * 7) & 255);
    const char four[] = {0x12, 0x30, 0x7f, 0x45, 0x60, 0x7f};
    short out[32];
    Converter::convert4BitTo16BitFlipMirror(four, out, 3, 2, 1, palette);
    const int expected[] = {4, 5, 6, 1, 2, 3};
    for (int i = 0; i < 6; ++i)
        CHECK((unsigned short)out[i] == paletteColor(palette[expected[i]]));

    const char eight[] = {1, 2, 99, 3, 4, 5, 99, 99};
    Converter::convert8BitTo16Bit(eight, out, 2, 2, 1, palette);
    CHECK((unsigned short)out[0] == paletteColor(palette[1]));
    CHECK((unsigned short)out[1] == paletteColor(palette[2]));
    CHECK((unsigned short)out[2] == paletteColor(palette[4]));
    CHECK((unsigned short)out[3] == paletteColor(palette[5]));
    Converter::convert8BitTo16BitFlipMirror(eight, out, 2, 2, 1, palette);
    CHECK((unsigned short)out[0] == paletteColor(palette[3]));
    CHECK((unsigned short)out[1] == paletteColor(palette[4]));
    CHECK((unsigned short)out[2] == paletteColor(palette[1]));
    CHECK((unsigned short)out[3] == paletteColor(palette[2]));

    const char high[] = {(char)0xff, (char)0x80, 0, 0};
    Converter::convert8BitTo16Bit(high, out, 2, 1, 0, palette);
    CHECK((unsigned short)out[0] == paletteColor(palette[255]));
    CHECK((unsigned short)out[1] == paletteColor(palette[128]));

    const unsigned char mono[] = {0xa5, 0xc0, 0x11, 0x3c, 0x80, 0x22};
    Converter::convert1BitTo16BitFlipMirror((const char*)mono, out, 10, 2, 1);
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 10; ++x)
            CHECK((unsigned short)out[(1 - y) * 10 + x]
                == ((mono[y * 3 + x / 8] >> (7 - x % 8)) & 1 ? 0xffff : 0));
}

static void packed_images()
{
    const short sixteen[] = {10, 20, 777, 30, 40, 888};
    short out[8];
    Converter::convert16BitTo16BitFlipMirror(sixteen, out, 2, 2, 1);
    CHECK(out[0] == 30 && out[1] == 40 && out[2] == 10 && out[3] == 20);
    const unsigned char pixels[] = {8, 16, 24, 248, 128, 64, 0x66, 0x77,
                                    32, 64, 128, 56, 24, 8, 0x66, 0x77};
    const char* in = (const char*)pixels;
    Converter::convert24BitTo16BitFlipMirror(in, out, 2, 2, 2);
    CHECK((unsigned short)out[0] == rgb(128, 64, 32));
    CHECK((unsigned short)out[1] == rgb(8, 24, 56));
    CHECK((unsigned short)out[2] == rgb(24, 16, 8));
    CHECK((unsigned short)out[3] == rgb(64, 128, 248));

    char shuffled[12];
    Converter::convert24BitTo24BitFlipMirrorColorShuffle(in, shuffled, 2, 2, 2);
    const unsigned char expected[] = {128, 64, 32, 8, 24, 56, 24, 16, 8, 64, 128, 248};
    for (int i = 0; i < 12; ++i)
        CHECK((unsigned char)shuffled[i] == expected[i]);

    Converter::convert24BitTo16BitColorShuffle(in, out, 2, 2, 2);
    CHECK((unsigned short)out[0] == rgb(248, 128, 64));
    CHECK((unsigned short)out[1] == rgb(8, 16, 24));
    CHECK((unsigned short)out[2] == rgb(56, 24, 8));
    CHECK((unsigned short)out[3] == rgb(32, 64, 128));
    Converter::convert24BitTo16BitFlipColorShuffle(in, out, 2, 2, 2);
    CHECK((unsigned short)out[0] == rgb(8, 16, 24));
    CHECK((unsigned short)out[1] == rgb(248, 128, 64));
    CHECK((unsigned short)out[2] == rgb(32, 64, 128));
    CHECK((unsigned short)out[3] == rgb(56, 24, 8));

    const unsigned char fourByte[] = {8, 16, 24, 64, 248, 128, 64, 0, 0x66,
                                     32, 64, 128, 255, 56, 24, 8, 0, 0x77};
    Converter::convert32BitTo16BitColorShuffle((const char*)fourByte, out, 2, 2, 1);
    CHECK((unsigned short)out[0] == rgb(64, 128, 248));
    CHECK((unsigned short)out[1] == rgb(24, 16, 8));
    CHECK((unsigned short)out[2] == rgb(8, 24, 56));
    CHECK((unsigned short)out[3] == rgb(128, 64, 32));
    for (int i = 0; i < 8; ++i)
        out[i] = 0x1234;
    Converter::convert32BitTo16BitFlipMirrorColorShuffle((const char*)fourByte, out, 2, 2, 1);
    CHECK(out[0] == 0x1234 && out[1] == 0x1234 && out[6] == 0x1234 && out[7] == 0x1234);
    CHECK((unsigned short)out[2] == rgb(128, 64, 32));
    CHECK((unsigned short)out[3] == rgb(8, 24, 56));
    CHECK((unsigned short)out[4] == rgb(24, 16, 8));
    CHECK((unsigned short)out[5] == rgb(64, 128, 248));

    const int words[] = {11, 22, 99, 33, 44, 88};
    int copied[4];
    Converter::convert32BitTo32BitFlipMirror(words, copied, 2, 2, 1);
    CHECK(copied[0] == 99 && copied[1] == 33 && copied[2] == 11 && copied[3] == 22);
}

static unsigned int expand(unsigned short color)
{
    return ((color & 0x8000u) << 16) | ((color & 0x7c00u) << 9)
        | ((color & 0x03e0u) << 6) | ((color & 0x001fu) << 3);
}

static void resized_image()
{
    const short in[] = {(short)0xfc00, (short)0x83e0, (short)0x801f, 0x7fff};
    int out[16];
    Converter::convert16bitToA8R8G8B8andResize(in, out, 4, 4, 2, 2);
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            CHECK((unsigned int)out[y * 4 + x] == expand(in[(y / 2) * 2 + x / 2]));
    out[0] = 0x12345678;
    Converter::convert16bitToA8R8G8B8andResize(in, out, 0, 4, 2, 2);
    Converter::convert16bitToA8R8G8B8andResize(in, out, 4, 0, 2, 2);
    CHECK(out[0] == 0x12345678);
}

static Format descriptor(const int* shifts)
{
    return (Format)((shifts[0] << 24) | (shifts[1] << 16) | (shifts[2] << 8) | shifts[3]);
}

static unsigned int reference(unsigned int value, const int* input, const int* output)
{
    unsigned int result = 0;
    for (int component = 0; component < 4; ++component)
        result |= ((value >> (24 - input[component])) & 255u) << (24 - output[component]);
    return result;
}

static void channel_permutations()
{
    const int words[] = {0x01020304, (int)0x89abcdefu, -1, 0, 0x10203080, (int)0x80000000u};
    int input[] = {0, 8, 16, 24};
    int pairs = 0;
    do
    {
        int output[] = {0, 8, 16, 24};
        do
        {
            int converted[8] = {0x1234, 0, 0, 0, 0, 0, 0, 0x1234};
            Converter::convert32BitTo32Bit(words, converted + 1, 6, descriptor(input), descriptor(output));
            CHECK(converted[0] == 0x1234 && converted[7] == 0x1234);
            int inplace[6];
            for (int i = 0; i < 6; ++i)
                inplace[i] = words[i];
            Converter::convert32BitTo32Bit(inplace, inplace, 6, descriptor(input), descriptor(output));
            for (int i = 0; i < 6; ++i)
            {
                unsigned int expected = reference(words[i], input, output);
                CHECK((unsigned int)converted[i + 1] == expected);
                CHECK((unsigned int)inplace[i] == expected);
                CHECK((unsigned int)Converter::convert32BitTo32Bit(words[i], descriptor(input), descriptor(output)) == expected);
            }
            ++pairs;
        } while (std::next_permutation(output, output + 4));
    } while (std::next_permutation(input, input + 4));
    CHECK(pairs == 576);
    int unchanged = 0x1234;
    Converter::convert32BitTo32Bit(words, &unchanged, 0, descriptor(input), descriptor(input));
    Converter::convert32BitTo32Bit(words, &unchanged, -1, descriptor(input), descriptor(input));
    CHECK(unchanged == 0x1234);
}

int main()
{
    indexed_images();
    packed_images();
    resized_image();
    channel_permutations();
    std::puts("Color-converter smoke passed: palette, mono, packed images, pitch quirks, resizing, 576 channel-format pairs and in-place conversion");
}
