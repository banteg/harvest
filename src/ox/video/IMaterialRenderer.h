// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IMaterialRenderer.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::video namespace; not the original source. The virtual order follows
// the Mac 1.18 vtable of ox::video::IMaterialRenderer.

#ifndef OX_VIDEO_IMATERIALRENDERER_H
#define OX_VIDEO_IMATERIALRENDERER_H

#include "../IUnknown.h"
#include "S3DVertex.h"
#include "SMaterial.h"

namespace ox {
namespace video {

class IMaterialRendererServices;

//! Sets the render states for one material type. The driver keeps them in its material renderer
//! table, indexed by E_MATERIAL_TYPE; shader materials are appended after the built-in types.
class IMaterialRenderer : public IUnknown
{
public:
    //! Called when the driver switches to this material.
    virtual void OnSetMaterial(SMaterial& material, const SMaterial& lastMaterial, bool resetAllRenderstates,
        IMaterialRendererServices* services)
    {
    }

    //! Called before each draw; false means the geometry should not be drawn.
    virtual bool OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype)
    {
        return true;
    }

    //! Called when the driver switches away from this material.
    virtual void OnUnsetMaterial()
    {
    }

    virtual bool isTransparent()
    {
        return false;
    }
};

} // end namespace video
} // end namespace ox

#endif
