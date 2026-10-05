// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The member names are inferred from their use.

#ifndef DAISY_VIDEO_CPARTICLESTATE_H
#define DAISY_VIDEO_CPARTICLESTATE_H

#include "ox/core/CPosition2d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/IParticleState.h"
#include "ox/video/SColor.h"

namespace ox {
namespace video {
class IParticlePackage;
class ISpriteAnimationState;
class ISpritePackage;
} // end namespace video
} // end namespace ox

namespace daisy {
namespace video {

struct SParticleFunction;
struct SParticleTypeInfo;

//! The running state of a particle function.
struct SParticleFunctionInstance
{
    SParticleFunctionInstance()
        : Time(0), Value(0)
    {
    }

    const SParticleFunction* Function;
    float Time;
    float Value;
};

//! A live particle of a particle type: its animation, motion, color, scale and pulses.
class CParticleState : public ox::video::IParticleState
{
public:
    CParticleState(ox::video::IParticlePackage* package, ox::video::ISpritePackage* spritePackage,
        SParticleTypeInfo* info);
    virtual ~CParticleState();

    virtual void remove();
    virtual bool update(float frameDelta, ox::core::CVector3d<float>& position);
    virtual void render2D(const ox::core::CPosition2d<float>& position, float scale);
    virtual void render2DShadow(const ox::core::CPosition2d<float>& position, float scale, float alpha);
    virtual void render3D(const ox::core::CVector3d<float>& position);
    virtual const ox::core::CVector3d<float>& getCurrentSpeed();
    virtual bool notifyBounce(ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& normal);
    virtual void addToSpeed(const ox::core::CVector3d<float>& speed);
    virtual void setCurrentSpeed(const ox::core::CVector3d<float>& speed);
    virtual const char* getParticleName();
    virtual const char* getAnimationName();
    virtual const ox::video::SColor& getCurrentColor();
    virtual float getCurrentScale();
    virtual bool isGroundSprite();
    virtual float getWindModifier() const;
    virtual int getParticleImportance() const;

private:
    //! Spawns the on-die particles at position.
    void createOnDieParticle(ox::core::CVector3d<float>& position);
    void updateParticleFunction(SParticleFunctionInstance& instance, float frameDelta);
    //! Spawns a random pulse particle at a random offset from position.
    void createPulseParticle(ox::core::CVector3d<float> position);
    //! Adds the parent's speed share of a spawned particle.
    void applyParentModifiers(CParticleState* parent);

    ox::video::IParticlePackage* Package;
    ox::video::ISpritePackage* SpritePackage;
    SParticleTypeInfo* Info;
    ox::video::ISpriteAnimationState* Animation;
    ox::core::CVector3d<float> Speed;
    int BounceCount;
    //! Seconds.
    float Age;
    float LifeTime;
    ox::video::SColor Color;
    float Scale;
    //! Radians.
    float Rotation;
    //! The last screen position for RotateToDirection; X is -1000 before the first one.
    ox::core::CPosition2d<float> LastPosition;
    //! Seconds until the next pulse starts.
    float PulseTimer;
    //! Seconds left of the running pulse.
    float PulseTimeLeft;
    float PulseInterval;
    //! Seconds until the running pulse spawns its next particle.
    float NextPulseParticle;
    int PulseCount;
    bool SoundPlayed;
    SParticleFunctionInstance MoveX;
    SParticleFunctionInstance MoveY;
    SParticleFunctionInstance MoveZ;
    SParticleFunctionInstance SpeedChange;
};

} // end namespace video
} // end namespace daisy

#endif
