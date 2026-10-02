// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CColorConverter.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef DAISY_VIDEO_CCOLORCONVERTER_H
#define DAISY_VIDEO_CCOLORCONVERTER_H

#include "ox/video/IVideoDriver.h"

namespace daisy {
namespace video {

class CColorConverter
{
public:
    static void convert4BitTo16BitFlipMirror(const char* in, short* out, int width, int height, int pitch, const int* palette);
    static void convert8BitTo16Bit(const char* in, short* out, int width, int height, int pitch, const int* palette);
    static void convert8BitTo16BitFlipMirror(const char* in, short* out, int width, int height, int pitch, const int* palette);
    static void convert1BitTo16BitFlipMirror(const char* in, short* out, int width, int height, int pitch);
    static void convert16BitTo16BitFlipMirror(const short* in, short* out, int width, int height, int pitch);
    static void convert24BitTo16BitFlipMirror(const char* in, short* out, int width, int height, int pitch);
    static void convert24BitTo24BitFlipMirrorColorShuffle(const char* in, char* out, int width, int height, int pitch);
    static void convert24BitTo16BitColorShuffle(const char* in, short* out, int width, int height, int pitch);
    static void convert24BitTo16BitFlipColorShuffle(const char* in, short* out, int width, int height, int pitch);
    static void convert32BitTo16BitColorShuffle(const char* in, short* out, int width, int height, int pitch);
    static void convert32BitTo16BitFlipMirrorColorShuffle(const char* in, short* out, int width, int height, int pitch);
    static void convert16bitToA8R8G8B8andResize(const short* in, int* out, int newWidth, int newHeight,
                                            int currentWidth, int currentHeight);
    static void convert32BitTo32BitFlipMirror(const int* in, int* out, int width, int height, int pitch);
    static void convert32BitTo32Bit(const int* in, int* out, int count,
                                   ox::video::ECOLOR_FORMAT input, ox::video::ECOLOR_FORMAT output);
    static int convert32BitTo32Bit(int color, ox::video::ECOLOR_FORMAT input, ox::video::ECOLOR_FORMAT output);
};

} // end namespace video
} // end namespace daisy

#endif
