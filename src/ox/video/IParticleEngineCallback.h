// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac vtable of ox::video::IParticleEngineCallback.

#ifndef OX_VIDEO_IPARTICLEENGINECALLBACK_H
#define OX_VIDEO_IPARTICLEENGINECALLBACK_H

#include "../core/CVector3d.h"

namespace ox {
namespace video {

class IParticleState;

//! Receives the particles and sounds a particle package spawns.
class IParticleEngineCallback
{
public:
    //! Takes over a new particle at a position.
    virtual void addParticleEntity(IParticleState* state, const core::CVector3d<float>& position) = 0;
    virtual int getOnDieMarkerAtPos(const core::CVector3d<float>& position) = 0;
    virtual void playParticleSound(const char* name, const core::CVector3d<float>& position) {}
};

} // end namespace video
} // end namespace ox

#endif
