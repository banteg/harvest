// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::video::CVideoNull (its third base). The
// game never reaches post-processing.

#ifndef OX_VIDEO_IPOSTPROCESSINGSERVICES_H
#define OX_VIDEO_IPOSTPROCESSINGSERVICES_H

#include "../core/CRect.h"

namespace ox {
namespace video {

class ITexture;

//! Full-screen post-processing: up to 8 screen-sized render surfaces, a screen capture into one of
//! them and shader passes that draw a surface back with a material.
class IPostProcessingServices
{
public:
    //! Grows or shrinks the surface set to count surfaces (at most 8).
    virtual void allocatePPSurfaces(unsigned int count) = 0;
    virtual void freePPSurfaces() = 0;
    //! Copies the whole screen into surface index.
    virtual void captureScreenBuffer(unsigned int index) = 0;
    virtual void captureScreenBuffer(unsigned int index, const core::CRect<int>& area) = 0;
    //! The surface at index, or null.
    virtual ITexture* getCapturedBuffer(unsigned int index) = 0;
    //! Runs material type over the whole screen.
    virtual void runPPShader(int materialType) = 0;
    virtual void runPPShader(int materialType, core::CRect<int>& destRect, core::CRect<int>* sourceRect,
        core::CRect<int>* clipRect, unsigned int sizeTextureStage) = 0;
    //! Sets the texture of stage 0 or 1 of the pass material; null selects surface stage.
    virtual void setInputTexture(int stage, ITexture* texture) = 0;
};

} // end namespace video
} // end namespace ox

#endif
