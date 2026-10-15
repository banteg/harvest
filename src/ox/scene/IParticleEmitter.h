// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IParticleEmitter.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef __I_PARTICLE_EMITTER_H_INCLUDED__
#define __I_PARTICLE_EMITTER_H_INCLUDED__

#include "ox/IUnknown.h"
#include "SParticle.h"

namespace ox
{
namespace scene
{

//! ox::core::A particle emitter for using with particle systems.
/** ox::core::A Particle emitter emitts new particles into a particle system.
*/
class IParticleEmitter : public ox::IUnknown
{
public:

	//! destructor
	virtual ~IParticleEmitter() {};

	//! Prepares an array with new particles to emitt into the system
	//! and returns how much new particles there are.
	//! \param now: Current time.
	//! \param timeSinceLastCall: Time elapsed since last call, in milliseconds.
	//! \param outArray: Pointer which will point to the array with the new
	//! particles to add into the system.
	//! \return Returns amount of new particles in the array. Can be 0.
	virtual int emitt(unsigned int now, unsigned int timeSinceLastCall, SParticle*& outArray) = 0;
};

} // end namespace scene
} // end namespace ox


#endif
