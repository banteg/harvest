// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CVideoModeList.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_VIDEO_CVIDEOMODELIST_H
#define DAISY_VIDEO_CVIDEOMODELIST_H

#include <vector>
#include "ox/video/IVideoModeList.h"

namespace daisy {
namespace video {

//! The video modes the device offers, kept sorted by addMode.
class CVideoModeList : public ox::video::IVideoModeList
{
public:
    CVideoModeList();
    virtual ~CVideoModeList();

    virtual int getVideoModeCount() const;
    virtual ox::core::CDimension2d<int> getVideoModeResolution(int modeNumber) const;
    virtual int getVideoModeDepth(int modeNumber) const;
    virtual ox::core::CDimension2d<int> getDesktopResolution() const;
    virtual int getDesktopDepth() const;

    void setDesktop(int desktopDepth, const ox::core::CDimension2d<int>& desktopSize);
    void addMode(const ox::core::CDimension2d<int>& size, int depth);

    struct SVideoMode
    {
        ox::core::CDimension2d<int> size;
        int depth;

        bool operator<(const SVideoMode& other) const;
    };

private:
    std::vector<SVideoMode> VideoModes;
    SVideoMode Desktop;
};

} // end namespace video
} // end namespace daisy

#endif
