// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: the virtual interface is not recovered yet; textures are reference counted.

#ifndef OX_VIDEO_ITEXTURE_H
#define OX_VIDEO_ITEXTURE_H

#include "../IUnknown.h"

namespace ox {
namespace video {

//! A texture of the video driver.
class ITexture : public IUnknown
{
};

} // end namespace video
} // end namespace ox

#endif
