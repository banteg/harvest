// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::video::COpenGLTexture; textures are
// reference counted.

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
    virtual const core::CDimension2d<int>& getSize() = 0;
    virtual int getDriverType() = 0;
    //! An ECOLOR_FORMAT value.
    virtual int getColorFormat() = 0;
    //! Bytes per row.
    virtual int getPitch() = 0;
};

} // end namespace video
} // end namespace ox

#endif
