// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IMaterialRenderer.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::video namespace; not the original source. The virtual order follows
// the Mac 1.18 vtable of daisy::video::COpenGLMaterialRenderer_SOLID; the Linux build emits the
// inline bodies below as COMDAT copies in CVideoOpenGL.o.

#ifndef OX_VIDEO_IMATERIALRENDERER_H
#define OX_VIDEO_IMATERIALRENDERER_H

#include "../IUnknown.h"
#include "S3DVertex.h"
#include "SMaterial.h"

namespace ox {
namespace video {

class IMaterialRendererServices;

//! Sets the render states of one material type. The video driver calls OnSetMaterial when a
//! material of this type becomes current, OnRender before each draw call and OnUnsetMaterial
//! when another material type takes over.
class IMaterialRenderer : public IUnknown
{
public:
    virtual ~IMaterialRenderer() {}

    virtual void OnSetMaterial(SMaterial& material, const SMaterial& lastMaterial, bool resetAllRenderstates,
        IMaterialRendererServices* services) {}

    //! Returns false to skip the draw call.
    virtual bool OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype) { return true; }

    virtual void OnUnsetMaterial() {}

    //! Transparent materials are drawn after the solid ones.
    virtual bool isTransparent() { return false; }
};

} // end namespace video
} // end namespace ox

#endif
