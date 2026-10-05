// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IMaterialRendererServices.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::video namespace; not the original source. The virtual order follows
// the Mac 1.18 vtable of daisy::video::CCGMaterialRenderer.

#ifndef OX_VIDEO_IMATERIALRENDERERSERVICES_H
#define OX_VIDEO_IMATERIALRENDERERSERVICES_H

#include "SMaterial.h"

namespace ox {
namespace video {

class IVideoDriver;

//! Lets material renderers and shader callbacks set render states and shader constants.
class IMaterialRendererServices
{
public:
    virtual ~IMaterialRendererServices() {}

    virtual void setBasicRenderStates(const SMaterial& material, const SMaterial& lastMaterial,
        bool resetAllRenderstates) = 0;
    virtual bool setVertexShaderConstant(const char* name, const float* floats, int count) = 0;
    virtual void setVertexShaderConstant(const float* data, int startRegister, int constantAmount = 1) = 0;
    virtual bool setPixelShaderConstant(const char* name, const float* floats, int count) = 0;
    virtual void setPixelShaderConstant(const float* data, int startRegister, int constantAmount = 1) = 0;
    virtual IVideoDriver* getVideoDriver() = 0;
};

} // end namespace video
} // end namespace ox

#endif
