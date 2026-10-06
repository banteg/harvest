// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/IImagePresenter.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_VIDEO_IIMAGEPRESENTER_H
#define DAISY_VIDEO_IIMAGEPRESENTER_H

namespace ox {
namespace video {
class IImage;
} // end namespace video
} // end namespace ox

namespace daisy {
namespace video {

//! Shows a software-rendered frame in the window; implemented by the device.
class IImagePresenter
{
public:
    virtual void present(ox::video::IImage* surface) = 0;
};

} // end namespace video
} // end namespace daisy

#endif
