// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CAnimatedMeshSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).

#ifndef PORT_SCENE_CANIMATEDMESHSCENENODE_H
#define PORT_SCENE_CANIMATEDMESHSCENENODE_H

#include <vector>
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IAnimatedMeshSceneNode.h"
#include "scene/SceneMath.h"

namespace daisy {
namespace scene {

//! Draws a mesh with per-buffer copies of its materials. The menu's planets and atmospheres are
//! these nodes; shadow volumes, MS3D joints and MD2 animations are not part of the port.
class CAnimatedMeshSceneNode : public ox::scene::IAnimatedMeshSceneNode
{
public:
    CAnimatedMeshSceneNode(ox::scene::IAnimatedMesh* mesh, ox::scene::ISceneNode* parent,
        ox::scene::ISceneManager* mgr, int id, const vector3df& position, const vector3df& rotation,
        const vector3df& scale);
    virtual ~CAnimatedMeshSceneNode();

    virtual void OnPreRender();
    virtual void OnPostRender(unsigned int timeMs);
    virtual void render();
    virtual const aabbox3df& getBoundingBox() const;
    virtual ox::video::SMaterial& getMaterial(int i);
    virtual int getMaterialCount();

    virtual void setCurrentFrame(int frame);
    virtual bool setFrameLoop(int begin, int end);
    virtual void setAnimationSpeed(int framesPerSecond);
    virtual ox::scene::IShadowVolumeSceneNode* addShadowVolumeSceneNode(int id, bool zfailmethod, float infinity);
    virtual ox::scene::ISceneNode* getMS3DJointNode(const char* jointName);
    virtual bool setMD2Animation(ox::scene::EMD2_ANIMATION_TYPE type);
    virtual bool setMD2Animation(const char* animationName);
    virtual int getFrameNr();

private:
    std::vector<ox::video::SMaterial> Materials;
    aabbox3df Box;
    ox::scene::IAnimatedMesh* Mesh;
    unsigned int BeginFrameTime;
    int StartFrame;
    int EndFrame;
    int FramesPerSecond;
};

} // end namespace scene
} // end namespace daisy

#endif
