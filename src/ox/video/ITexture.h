// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: the virtual interface is recovered up to getSize, in Irrlicht 0.7 order.

#ifndef OX_VIDEO_ITEXTURE_H
#define OX_VIDEO_ITEXTURE_H

#include "../IUnknown.h"
#include "../core/CDimension2d.h"

namespace ox {
namespace video {

//! A texture of the video driver.
class ITexture : public IUnknown
{
public:
    virtual void* lock() = 0;
    virtual void unlock() = 0;
    virtual const core::CDimension2d<int>& getOriginalSize() = 0;
    //! The size of the texture in video memory, which may be larger than the original image.
    virtual const core::CDimension2d<int>& getSize() = 0;
};

} // end namespace video
} // end namespace ox

#endif
