// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CVideoModeList.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_VIDEO_CVIDEOMODELIST_H
#define DAISY_VIDEO_CVIDEOMODELIST_H

#include <vector>
#include "ox/video/IVideoModeList.h"

namespace daisy {
namespace video {

//! The video modes the device offers, kept sorted ascending (see SVideoMode::operator<) by addMode.
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
    //! Adds a mode unless it is already listed, then re-sorts the list.
    void addMode(const ox::core::CDimension2d<int>& size, int depth);

    struct SVideoMode
    {
        ox::core::CDimension2d<int> size;
        int depth;

        bool operator==(const SVideoMode& other) const
        {
            return size == other.size && depth == other.depth;
        }

        //! Ascending by width, then height, then depth.
        bool operator<(const SVideoMode& other) const
        {
            return (size.Width < other.size.Width
                || (size.Width == other.size.Width && size.Height < other.size.Height)
                || (size.Width == other.size.Width && size.Height == other.size.Height && depth < other.depth));
        }
    };

private:
    std::vector<SVideoMode> VideoModes;
    SVideoMode Desktop;
};

} // end namespace video
} // end namespace daisy

#endif
