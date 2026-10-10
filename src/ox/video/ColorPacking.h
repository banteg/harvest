// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/SColor.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest; the separate header avoids changing unrelated units' compiler context.

#ifndef OX_VIDEO_COLORPACKING_H
#define OX_VIDEO_COLORPACKING_H

namespace ox {
namespace video {

inline short RGB16(int r, int g, int b)
{
    return 0x8000 | (((r >> 3) & 0x1f) << 10) | (((g >> 3) & 0x1f) << 5) | ((b >> 3) & 0x1f);
}

inline short X8R8G8B8toA1R5G5B5(int color)
{
    return RGB16(color >> 16, color >> 8, color);
}

inline int getBlue(short color)
{
    return ((color)&0x1F);
}

inline int getGreen(short color)
{
    return ((color >> 5)&0x1F);
}

inline int getRed(short color)
{
    return ((color >> 10)&0x1F);
}

} // end namespace video
} // end namespace ox

#endif
