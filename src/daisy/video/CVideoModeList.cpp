// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CVideoModeList.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#include "CVideoModeList.h"
#include "ox/algo/CArrayFunctions.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace video {

CVideoModeList::CVideoModeList()
    : Desktop()
{
}

CVideoModeList::~CVideoModeList()
{
}

void CVideoModeList::setDesktop(int desktopDepth, const ox::core::CDimension2d<int>& desktopSize)
{
    Desktop.depth = desktopDepth;
    Desktop.size = desktopSize;
}

int CVideoModeList::getVideoModeCount() const
{
    return (int)VideoModes.size();
}

//! Original bug: the bound check accepts modeNumber == count and then reads one entry past the end.
ox::core::CDimension2d<int> CVideoModeList::getVideoModeResolution(int modeNumber) const
{
    if (modeNumber < 0 || modeNumber > (int)VideoModes.size())
        return ox::core::CDimension2d<int>(0, 0);

    return VideoModes[modeNumber].size;
}

//! Original bug: the same off-by-one bound as getVideoModeResolution.
int CVideoModeList::getVideoModeDepth(int modeNumber) const
{
    if (modeNumber < 0 || modeNumber > (int)VideoModes.size())
        return 0;

    return VideoModes[modeNumber].depth;
}

ox::core::CDimension2d<int> CVideoModeList::getDesktopResolution() const
{
    return Desktop.size;
}

int CVideoModeList::getDesktopDepth() const
{
    return Desktop.depth;
}

void CVideoModeList::addMode(const ox::core::CDimension2d<int>& size, int depth)
{
    SVideoMode m;
    m.depth = depth;
    m.size = size;

    for (int i = 0; i < (int)VideoModes.size(); ++i)
        if (VideoModes[i] == m)
            return;

    VideoModes.push_back(m);
    ox::algo::sort(VideoModes.begin(), VideoModes.end());
}

} // end namespace video
} // end namespace daisy
