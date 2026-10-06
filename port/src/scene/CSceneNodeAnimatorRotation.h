// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneNodeAnimatorRotation.{h,cpp} (license:
// third_party/irrlicht-0.7/include/irrlicht.h). Unchanged in daisy (Mac 0x11727a, Linux 0x4cc1a0).

#ifndef PORT_SCENE_CSCENENODEANIMATORROTATION_H
#define PORT_SCENE_CSCENENODEANIMATORROTATION_H

#include "ox/scene/ISceneNode.h"
#include "ox/scene/ISceneNodeAnimator.h"
#include "scene/SceneMath.h"

namespace daisy {
namespace scene {

//! Adds rotation * (elapsed milliseconds / 10) to the node's rotation each frame, so the rotation
//! is in degrees per 10 ms (the menu's -0.005 is -0.5 degrees per second).
class CSceneNodeAnimatorRotation : public ox::scene::ISceneNodeAnimator
{
public:
    CSceneNodeAnimatorRotation(unsigned int time, const vector3df& rotation)
        : Rotation(rotation), StartTime(time)
    {
    }

    virtual void animateNode(ox::scene::ISceneNode* node, unsigned int timeMs)
    {
        if (!node)
            return;

        vector3df newRotation = node->getRotation();
        newRotation += Rotation * ((timeMs - StartTime) / 10.0f);
        node->setRotation(newRotation);
        StartTime = timeMs;
    }

private:
    vector3df Rotation;
    unsigned int StartTime;
};

} // end namespace scene
} // end namespace daisy

#endif
