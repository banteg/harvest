// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the third vtable of daisy::video::CVideoOpenGL (Mac and Linux 1.18).
// Provisional: the return types are not verified; the game never uses post-processing.

#ifndef OX_VIDEO_IPOSTPROCESSINGSERVICES_H
#define OX_VIDEO_IPOSTPROCESSINGSERVICES_H

#include "../core/CRect.h"

namespace ox {
namespace video {

class ITexture;

//! Screen capture buffers and full screen shader passes of the video driver.
class IPostProcessingServices
{
public:
    virtual bool allocatePPSurfaces(unsigned int count) = 0;
    virtual void freePPSurfaces() = 0;
    virtual void captureScreenBuffer(unsigned int index) = 0;
    virtual void captureScreenBuffer(unsigned int index, const core::CRect<int>& area) = 0;
    virtual ITexture* getCapturedBuffer(unsigned int index) = 0;
    virtual void runPPShader(int material) = 0;
    virtual void runPPShader(int material, core::CRect<int>& destRect, core::CRect<int>* sourceRect,
        core::CRect<int>* clipRect, unsigned int index) = 0;
    virtual void setInputTexture(int index, ITexture* texture) = 0;
};

} // end namespace video
} // end namespace ox

#endif
