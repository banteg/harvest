// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IBillboardSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef OX_SCENE_IBILLBOARDSCENENODE_H
#define OX_SCENE_IBILLBOARDSCENENODE_H

#include "ISceneNode.h"
#include "../core/CDimension2d.h"

namespace ox {
namespace scene {

//! A quad that always faces the camera.
class IBillboardSceneNode : public ISceneNode
{
public:
    virtual void setSize(const core::CDimension2d<float>& size) = 0;
    virtual const core::CDimension2d<float>& getSize() = 0;
};

} // end namespace scene
} // end namespace ox

#endif
