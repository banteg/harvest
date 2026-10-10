// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IParticleAffector.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef __I_PARTICLE_AFFECTOR_H_INCLUDED__
#define __I_PARTICLE_AFFECTOR_H_INCLUDED__

#include "ox/IUnknown.h"
#include "ox/scene/SParticle.h"

namespace ox
{
namespace scene
{

//! A particle affector modifies particles.
class IParticleAffector : public ox::IUnknown
{
public:

	//! Affects an array of particles.
	//! \param now: Current time. (Same as ITimer::getTime() would return)
	//! \param particlearray: Array of particles.
	//! \param count: Amount of particles in array.
	virtual void affect(unsigned int now, SParticle* particlearray, unsigned int count) = 0;

};

} // end namespace scene
} // end namespace ox


#endif
