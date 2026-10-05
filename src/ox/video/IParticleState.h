// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual interface follows the Mac vtable (names from daisy::video::CParticleState); the return
// types follow the Linux daisy::video::CParticleState.

#ifndef OX_VIDEO_IPARTICLESTATE_H
#define OX_VIDEO_IPARTICLESTATE_H

#include "../IUnknown.h"
#include "../core/CPosition2d.h"
#include "../core/CString.h"
#include "../core/CVector3d.h"
#include "SColor.h"

namespace ox {
namespace video {

//! A live particle: its sprite, motion and lifetime.
class IParticleState : public IUnknown
{
public:
    virtual void remove() = 0;
    //! Moves the particle; false once it has died.
    virtual bool update(float frameDelta, core::CVector3d<float>& position) = 0;
    virtual void render2D(const core::CPosition2d<float>& position, float scale) = 0;
    virtual void render2DShadow(const core::CPosition2d<float>& position, float scale, float alpha) = 0;
    virtual void render3D(const core::CVector3d<float>& position) = 0;
    virtual const core::CVector3d<float>& getCurrentSpeed() = 0;
    //! Bounces off a surface; true when the particle stops.
    virtual bool notifyBounce(core::CVector3d<float>& position, const core::CVector3d<float>& normal) = 0;
    virtual void addToSpeed(const core::CVector3d<float>& speed) = 0;
    virtual void setCurrentSpeed(const core::CVector3d<float>& speed) = 0;
    virtual const char* getParticleName() = 0;
    //! The first animation name of the particle type, or "".
    virtual const char* getAnimationName() = 0;
    virtual const SColor& getCurrentColor() = 0;
    virtual float getCurrentScale() = 0;
    virtual bool isGroundSprite() = 0;
    virtual float getWindModifier() const = 0;
    virtual int getParticleImportance() const = 0;
};

} // end namespace video
} // end namespace ox

#endif
