// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CAnimatedMeshSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
//
// Checked against the Mac 1.18 build (0xf553a-0xf62b6). daisy's OnPostRender (0xf59ea) runs the
// animators and the children but no longer updates the node's absolute transformation: that now
// happens in ISceneNode::OnPreRender, after the node has registered for rendering.

#include "scene/CAnimatedMeshSceneNode.h"
#include "daisy/os.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/IMeshBuffer.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNodeAnimator.h"
#include "ox/video/IVideoDriver.h"

namespace daisy {
namespace scene {

CAnimatedMeshSceneNode::CAnimatedMeshSceneNode(ox::scene::IAnimatedMesh* mesh, ox::scene::ISceneNode* parent,
    ox::scene::ISceneManager* mgr, int id, const vector3df& position, const vector3df& rotation,
    const vector3df& scale)
    : ox::scene::IAnimatedMeshSceneNode(parent, mgr, id, position, rotation, scale), Mesh(mesh), BeginFrameTime(0),
      StartFrame(0), EndFrame(0), FramesPerSecond(100)
{
    BeginFrameTime = os::Timer::getTime();

    if (!Mesh)
        return;

    Box = Mesh->getBoundingBox();

    ox::scene::IMesh* m = Mesh->getMesh(0, 0);
    if (m)
    {
        ox::video::SMaterial mat;
        for (int i = 0; i < m->getMeshBufferCount(); ++i)
        {
            ox::scene::IMeshBuffer* mb = m->getMeshBuffer(i);
            if (mb)
                mat = mb->getMaterial();
            Materials.push_back(mat);
        }
    }

    StartFrame = 0;
    EndFrame = Mesh->getFrameCount();
    Mesh->grab();
}

CAnimatedMeshSceneNode::~CAnimatedMeshSceneNode()
{
    if (Mesh)
        Mesh->drop();
}

void CAnimatedMeshSceneNode::OnPreRender()
{
    if (IsVisible)
        SceneManager->registerNodeForRendering(this, ox::scene::SNRT_DEFAULT);

    ISceneNode::OnPreRender();
}

int CAnimatedMeshSceneNode::getFrameNr()
{
    if (EndFrame == StartFrame)
        return StartFrame;

    return StartFrame + ((int)((os::Timer::getTime() - BeginFrameTime) * (FramesPerSecond / 1000.0f)) %
        (EndFrame - StartFrame));
}

void CAnimatedMeshSceneNode::OnPostRender(unsigned int timeMs)
{
    if (!IsVisible)
        return;

    for (ox::TList<ox::scene::ISceneNodeAnimator*>::iterator it = Animators.begin(); it != Animators.end(); ++it)
        (*it)->animateNode(this, timeMs);

    for (ox::TList<ox::scene::ISceneNode*>::iterator it = Children.begin(); it != Children.end(); ++it)
        (*it)->OnPostRender(timeMs);
}

//! World transform, then each mesh buffer with its material copy.
void CAnimatedMeshSceneNode::render()
{
    ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();
    if (!Mesh || !driver)
        return;

    driver->setTransform(ox::video::ETS_WORLD, AbsoluteTransformation);

    int frame = getFrameNr();
    ox::scene::IMesh* m = Mesh->getMesh(frame, 255, StartFrame, EndFrame);
    if (!m)
        return;

    Box = m->getBoundingBox();

    for (int i = 0; i < m->getMeshBufferCount(); ++i)
    {
        ox::scene::IMeshBuffer* mb = m->getMeshBuffer(i);
        driver->setMaterial(Materials[i]);
        driver->drawMeshBuffer(mb);
    }
}

const aabbox3df& CAnimatedMeshSceneNode::getBoundingBox() const
{
    return Box;
}

ox::video::SMaterial& CAnimatedMeshSceneNode::getMaterial(int i)
{
    if (i < 0 || i >= (int)Materials.size())
        return ISceneNode::getMaterial(i);

    return Materials[i];
}

int CAnimatedMeshSceneNode::getMaterialCount()
{
    return (int)Materials.size();
}

void CAnimatedMeshSceneNode::setCurrentFrame(int frame)
{
}

bool CAnimatedMeshSceneNode::setFrameLoop(int begin, int end)
{
    if (!Mesh)
        return false;

    int frameCount = Mesh->getFrameCount();
    if (!(begin <= end && begin < frameCount && end < frameCount))
        return false;

    StartFrame = begin;
    EndFrame = end;
    BeginFrameTime = os::Timer::getTime();
    return true;
}

void CAnimatedMeshSceneNode::setAnimationSpeed(int framesPerSecond)
{
    FramesPerSecond = framesPerSecond;
}

ox::scene::IShadowVolumeSceneNode* CAnimatedMeshSceneNode::addShadowVolumeSceneNode(int id, bool zfailmethod,
    float infinity)
{
    return 0;
}

ox::scene::ISceneNode* CAnimatedMeshSceneNode::getMS3DJointNode(const char* jointName)
{
    return 0;
}

bool CAnimatedMeshSceneNode::setMD2Animation(ox::scene::EMD2_ANIMATION_TYPE type)
{
    return false;
}

bool CAnimatedMeshSceneNode::setMD2Animation(const char* animationName)
{
    return false;
}

} // end namespace scene
} // end namespace daisy
