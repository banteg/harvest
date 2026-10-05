// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/ICameraSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source. The virtual order follows
// the Mac 1.18 vtable of daisy::scene::CCameraSceneNode.

#ifndef OX_SCENE_ICAMERASCENENODE_H
#define OX_SCENE_ICAMERASCENENODE_H

#include "ISceneNode.h"
#include "../event/IEventReceiver.h"

namespace ox {
namespace scene {

struct SViewFrustrum;

//! A camera.
class ICameraSceneNode : public ISceneNode
{
public:
    virtual void setProjectionMatrix(const core::CMatrix4& projection) = 0;
    virtual const core::CMatrix4& getProjectionMatrix() = 0;
    virtual const core::CMatrix4& getViewMatrix() = 0;
    virtual bool OnEvent(const event::SEvent& event) = 0;
    virtual void setTarget(const core::CVector3d<float>& position) = 0;
    virtual core::CVector3d<float> getTarget() const = 0;
    virtual void setUpVector(const core::CVector3d<float>& position) = 0;
    virtual core::CVector3d<float> getUpVector() const = 0;
    virtual float getNearValue() = 0;
    virtual float getFarValue() = 0;
    virtual float getAspectRatio() = 0;
    virtual float getFOV() = 0;
    virtual void setNearValue(float zn) = 0;
    virtual void setFarValue(float zf) = 0;
    virtual void setAspectRatio(float aspect) = 0;
    virtual void setFOV(float fovy) = 0;
    virtual const SViewFrustrum* getViewFrustrum() = 0;
    virtual void setInputReceiverEnabled(bool enabled) = 0;
    virtual bool isInputReceiverEnabled() = 0;
};

} // end namespace scene
} // end namespace ox

#endif
