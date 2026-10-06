// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CCameraSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).

#ifndef PORT_SCENE_CCAMERASCENENODE_H
#define PORT_SCENE_CCAMERASCENENODE_H

#include "ox/core/CDimension2d.h"
#include "ox/scene/ICameraSceneNode.h"
#include "scene/SViewFrustrum.h"

namespace daisy {
namespace scene {

//! A camera looking from its absolute position at a target point.
class CCameraSceneNode : public ox::scene::ICameraSceneNode
{
public:
    CCameraSceneNode(ox::scene::ISceneNode* parent, ox::scene::ISceneManager* mgr, int id,
        const vector3df& position, const vector3df& lookat);

    virtual void OnPreRender();
    virtual void render();
    virtual const aabbox3df& getBoundingBox() const;

    virtual void setProjectionMatrix(const matrix4& projection);
    virtual const matrix4& getProjectionMatrix();
    virtual const matrix4& getViewMatrix();
    virtual bool OnEvent(const ox::event::SEvent& event);
    virtual void setTarget(const vector3df& position);
    virtual vector3df getTarget() const;
    virtual void setUpVector(const vector3df& position);
    virtual vector3df getUpVector() const;
    virtual float getNearValue();
    virtual float getFarValue();
    virtual float getAspectRatio();
    virtual float getFOV();
    virtual void setNearValue(float zn);
    virtual void setFarValue(float zf);
    virtual void setAspectRatio(float aspect);
    virtual void setFOV(float fovy);
    virtual const ox::scene::SViewFrustrum* getViewFrustrum();
    virtual void setInputReceiverEnabled(bool enabled);
    virtual bool isInputReceiverEnabled();

private:
    void recalculateProjectionMatrix();
    void recalculateViewArea();

    vector3df Target;
    vector3df UpVector;
    matrix4 Projection;
    matrix4 View;
    aabbox3df BBox;
    //! Radians.
    float Fovy;
    //! Height / width of the screen at creation, as in Irrlicht 0.7.
    float Aspect;
    float ZNear;
    float ZFar;
    ox::core::CDimension2d<float> ScreenDim;
    ox::scene::SViewFrustrum ViewArea;
    bool InputReceiverEnabled;
};

} // end namespace scene
} // end namespace daisy

#endif
