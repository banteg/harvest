// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IAnimatedMesh.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source. Partial: the mesh type is not
// recovered; the slots CGUIMeshViewer calls are verified against the Mac 1.18 build.

#ifndef OX_SCENE_IANIMATEDMESH_H
#define OX_SCENE_IANIMATEDMESH_H

#include "../IUnknown.h"

namespace ox {
namespace core {
template <class T> class CAabbox3d;
} // end namespace core

namespace scene {

class IMesh;

//! A mesh with one static mesh per frame.
class IAnimatedMesh : public IUnknown
{
public:
    virtual ~IAnimatedMesh() {}

    virtual int getFrameCount() = 0;
    virtual IMesh* getMesh(int frame, int detailLevel = 255, int startFrameLoop = -1, int endFrameLoop = -1) = 0;
    virtual const core::CAabbox3d<float>& getBoundingBox() const = 0;
};

} // end namespace scene
} // end namespace ox

#endif
