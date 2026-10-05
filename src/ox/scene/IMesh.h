// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IMesh.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source. The slots CGUIMeshViewer
// calls are verified against the Mac 1.18 build.

#ifndef OX_SCENE_IMESH_H
#define OX_SCENE_IMESH_H

#include "../IUnknown.h"
#include "../video/SMaterial.h"

namespace ox {
namespace core {
template <class T> class CAabbox3d;
} // end namespace core

namespace scene {

class IMeshBuffer;

//! A static mesh made of mesh buffers.
class IMesh : public IUnknown
{
public:
    virtual ~IMesh() {}

    virtual int getMeshBufferCount() = 0;
    virtual IMeshBuffer* getMeshBuffer(int nr) = 0;
    virtual const core::CAabbox3d<float>& getBoundingBox() const = 0;
    virtual core::CAabbox3d<float>& getBoundingBox() = 0;
    virtual void setMaterialFlag(video::E_MATERIAL_FLAG flag, bool newvalue) = 0;
};

} // end namespace scene
} // end namespace ox

#endif
