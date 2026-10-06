// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IAnimatedMeshSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source. The virtual order follows
// the Mac 1.18 vtable of daisy::scene::CAnimatedMeshSceneNode.

#ifndef OX_SCENE_IANIMATEDMESHSCENENODE_H
#define OX_SCENE_IANIMATEDMESHSCENENODE_H

#include "ISceneNode.h"

namespace ox {
namespace scene {

class IShadowVolumeSceneNode;

//! Animation types of MD2 meshes; the enumerators are not recovered.
enum EMD2_ANIMATION_TYPE
{
};

//! A scene node that displays an animated mesh.
class IAnimatedMeshSceneNode : public ISceneNode
{
public:
#ifdef HARVEST_PORT
    IAnimatedMeshSceneNode(ISceneNode* parent, ISceneManager* mgr, int id,
        const core::CVector3d<float>& position = core::CVector3d<float>(0, 0, 0),
        const core::CVector3d<float>& rotation = core::CVector3d<float>(0, 0, 0),
        const core::CVector3d<float>& scale = core::CVector3d<float>(1.0f, 1.0f, 1.0f))
        : ISceneNode(parent, mgr, id, position, rotation, scale)
    {
    }

#endif
    virtual void setCurrentFrame(int frame) = 0;
    virtual bool setFrameLoop(int begin, int end) = 0;
    virtual void setAnimationSpeed(int framesPerSecond) = 0;
    virtual IShadowVolumeSceneNode* addShadowVolumeSceneNode(int id, bool zfailmethod, float infinity) = 0;
    virtual ISceneNode* getMS3DJointNode(const char* jointName) = 0;
    virtual bool setMD2Animation(EMD2_ANIMATION_TYPE type) = 0;
    virtual bool setMD2Animation(const char* animationName) = 0;
    virtual int getFrameNr() = 0;
};

} // end namespace scene
} // end namespace ox

#endif
