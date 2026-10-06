// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CCameraSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
//
// Checked against the Mac 1.18 build (constructor 0xfb9ca, recalculateProjectionMatrix 0xfbdc2,
// OnPreRender 0xfc162). Differences from Irrlicht 0.7: OnPreRender first updates the camera's
// absolute transformation, so the view uses the position the game set this frame, and it uploads
// the projection after building the view frustum instead of before.

#include "scene/CCameraSceneNode.h"
#include "ox/scene/ISceneManager.h"
#include "ox/video/IVideoDriver.h"

namespace daisy {
namespace scene {

CCameraSceneNode::CCameraSceneNode(ox::scene::ISceneNode* parent, ox::scene::ISceneManager* mgr, int id,
    const vector3df& position, const vector3df& lookat)
    : ox::scene::ICameraSceneNode(parent, mgr, id, position, vector3df(0.0f, 0.0f, 0.0f), vector3df(1.0f, 1.0f, 1.0f)),
      InputReceiverEnabled(true)
{
    resetBox(BBox, vector3df(0, 0, 0));
    UpVector.set(0.0f, 1.0f, 0.0f);
    Target.set(lookat);

    Fovy = 3.14159265358979f / 2.5f;
    Aspect = 4.0f / 3.0f;
    ZNear = 1.0f;
    ZFar = 3000.0f;

    ox::video::IVideoDriver* d = mgr->getVideoDriver();
    if (d)
    {
        ScreenDim.Width = (float)d->getScreenSize().Width;
        ScreenDim.Height = (float)d->getScreenSize().Height;
        Aspect = ScreenDim.Height / ScreenDim.Width;
    }

    recalculateProjectionMatrix();
}

void CCameraSceneNode::setInputReceiverEnabled(bool enabled)
{
    InputReceiverEnabled = enabled;
}

bool CCameraSceneNode::isInputReceiverEnabled()
{
    return InputReceiverEnabled;
}

void CCameraSceneNode::setProjectionMatrix(const matrix4& projection)
{
    Projection = projection;
}

const matrix4& CCameraSceneNode::getProjectionMatrix()
{
    return Projection;
}

const matrix4& CCameraSceneNode::getViewMatrix()
{
    return View;
}

bool CCameraSceneNode::OnEvent(const ox::event::SEvent& event)
{
    return false;
}

void CCameraSceneNode::setTarget(const vector3df& position)
{
    Target = position;
}

vector3df CCameraSceneNode::getTarget() const
{
    return Target;
}

void CCameraSceneNode::setUpVector(const vector3df& position)
{
    UpVector = position;
}

vector3df CCameraSceneNode::getUpVector() const
{
    return UpVector;
}

float CCameraSceneNode::getNearValue()
{
    return ZNear;
}

float CCameraSceneNode::getFarValue()
{
    return ZFar;
}

float CCameraSceneNode::getAspectRatio()
{
    return Aspect;
}

float CCameraSceneNode::getFOV()
{
    return Fovy;
}

void CCameraSceneNode::setNearValue(float zn)
{
    ZNear = zn;
    recalculateProjectionMatrix();
}

void CCameraSceneNode::setFarValue(float zf)
{
    ZFar = zf;
    recalculateProjectionMatrix();
}

void CCameraSceneNode::setAspectRatio(float aspect)
{
    Aspect = aspect;
    recalculateProjectionMatrix();
}

void CCameraSceneNode::setFOV(float fovy)
{
    Fovy = fovy;
    recalculateProjectionMatrix();
}

void CCameraSceneNode::recalculateProjectionMatrix()
{
    buildProjectionMatrixPerspectiveFovLH(Projection, Fovy, Aspect, ZNear, ZFar);
}

void CCameraSceneNode::OnPreRender()
{
    ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();
    if (!driver)
        return;

    updateAbsolutePosition();

    if (SceneManager->getActiveCamera() == this)
    {
        ScreenDim.Width = (float)driver->getScreenSize().Width;
        ScreenDim.Height = (float)driver->getScreenSize().Height;

        vector3df pos = getAbsolutePosition();
        vector3df tgtv = Target - pos;
        tgtv.normalize();

        vector3df up = UpVector;
        up.normalize();

        // Keep the look-at basis defined when looking along the up vector.
        float dp = tgtv.dotProduct(up);
        if ((dp > -1.0001f && dp < -0.9999f) || (dp < 1.0001f && dp > 0.9999f))
            up.X += 1.0f;

        buildCameraLookAtMatrixLH(View, pos, Target, up);
        recalculateViewArea();

        driver->setTransform(ox::video::ETS_PROJECTION, Projection);
        SceneManager->registerNodeForRendering(this, ox::scene::SNRT_LIGHT_AND_CAMERA);
    }

    ISceneNode::OnPreRender();
}

//! Sets the view transform; drawAll renders the camera before anything else.
void CCameraSceneNode::render()
{
    ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();
    if (!driver)
        return;

    driver->setTransform(ox::video::ETS_VIEW, View);
}

const aabbox3df& CCameraSceneNode::getBoundingBox() const
{
    return BBox;
}

const ox::scene::SViewFrustrum* CCameraSceneNode::getViewFrustrum()
{
    return &ViewArea;
}

void CCameraSceneNode::recalculateViewArea()
{
    matrix4 mat = Projection * View;
    ViewArea = ox::scene::SViewFrustrum(mat);
    ViewArea.cameraPosition = getAbsolutePosition();
    ViewArea.recalculateBoundingBox();
}

} // end namespace scene
} // end namespace daisy
