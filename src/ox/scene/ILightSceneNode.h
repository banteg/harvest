// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/ILightSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef OX_SCENE_ILIGHTSCENENODE_H
#define OX_SCENE_ILIGHTSCENENODE_H

#include "ISceneNode.h"
#include "../video/SLight.h"

namespace ox {
namespace scene {

//! A dynamic light.
class ILightSceneNode : public ISceneNode
{
public:
#ifdef HARVEST_PORT
    ILightSceneNode(ISceneNode* parent, ISceneManager* mgr, int id,
        const core::CVector3d<float>& position = core::CVector3d<float>(0, 0, 0))
        : ISceneNode(parent, mgr, id, position)
    {
    }

#endif
    virtual video::SLight& getLightData() = 0;
};

} // end namespace scene
} // end namespace ox

#endif
