// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CLightSceneNode.{h,cpp} (license: third_party/irrlicht-0.7/include/irrlicht.h).

#ifndef PORT_SCENE_CLIGHTSCENENODE_H
#define PORT_SCENE_CLIGHTSCENENODE_H

#include "ox/scene/ILightSceneNode.h"
#include "scene/SceneMath.h"

namespace daisy {
namespace scene {

//! A point light added to the driver's dynamic lights every frame.
class CLightSceneNode : public ox::scene::ILightSceneNode
{
public:
    CLightSceneNode(ox::scene::ISceneNode* parent, ox::scene::ISceneManager* mgr, int id, const vector3df& position,
        ox::video::SColorf color, float radius);

    virtual void OnPreRender();
    virtual void render();
    virtual const aabbox3df& getBoundingBox() const;
    virtual ox::video::SLight& getLightData();

private:
    ox::video::SLight LightData;
    aabbox3df BBox;
};

} // end namespace scene
} // end namespace daisy

#endif
