// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IShaderConstantSetCallBack.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::video namespace; not the original source.

#ifndef OX_VIDEO_ISHADERCONSTANTSETCALLBACK_H
#define OX_VIDEO_ISHADERCONSTANTSETCALLBACK_H

#include "../IUnknown.h"

namespace ox {
namespace video {

class IMaterialRendererServices;

//! Sets the shader constants of a material before it is drawn.
class IShaderConstantSetCallBack : public virtual IUnknown
{
public:
    virtual ~IShaderConstantSetCallBack() {}

    virtual void OnSetConstants(IMaterialRendererServices* services, int userData) = 0;
};

} // end namespace video
} // end namespace ox

#endif
