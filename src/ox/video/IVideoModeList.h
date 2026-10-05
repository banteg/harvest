// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IVideoModeList.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::video namespace; not the original source.

#ifndef OX_VIDEO_IVIDEOMODELIST_H
#define OX_VIDEO_IVIDEOMODELIST_H

#include "../IUnknown.h"
#include "../core/CDimension2d.h"

namespace ox {
namespace video {

//! A list of all available video modes.
class IVideoModeList : public IUnknown
{
public:
    virtual int getVideoModeCount() const = 0;
    virtual core::CDimension2d<int> getVideoModeResolution(int modeNumber) const = 0;
    //! The color depth of a mode in bits.
    virtual int getVideoModeDepth(int modeNumber) const = 0;
    virtual core::CDimension2d<int> getDesktopResolution() const = 0;
    virtual int getDesktopDepth() const = 0;
};

} // end namespace video
} // end namespace ox

#endif
