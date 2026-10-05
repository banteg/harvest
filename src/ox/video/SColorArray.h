// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef OX_VIDEO_SCOLORARRAY_H
#define OX_VIDEO_SCOLORARRAY_H

#include "SColor.h"

namespace ox {
namespace video {

//! The vertex colors of a drawn quad, counterclockwise from the upper left corner.
struct SColorArray
{
    SColorArray() {}

    SColorArray(SColor color)
    {
        Colors[0] = color;
        Colors[1] = color;
        Colors[2] = color;
        Colors[3] = color;
    }

    SColorArray(SColor upperLeft, SColor lowerLeft, SColor lowerRight, SColor upperRight)
    {
        Colors[0] = upperLeft;
        Colors[1] = lowerLeft;
        Colors[2] = lowerRight;
        Colors[3] = upperRight;
    }

    SColor Colors[4];
};

//! Colors on a 3x3 grid over a quad (corners, edge midpoints and center), row by row from the top.
struct SColorCorners
{
    SColor Colors[9];
};

} // end namespace video
} // end namespace ox

#endif
