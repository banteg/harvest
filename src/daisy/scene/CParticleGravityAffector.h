// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CParticleGravityAffector.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_PARTICLE_GRAVITY_AFFECTOR_H_INCLUDED__
#define __C_PARTICLE_GRAVITY_AFFECTOR_H_INCLUDED__

#include "ox/scene/IParticleAffector.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/SParticle.h"
#include "ox/video/SColor.h"
#include "ox/video/ColorPacking.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! Particle Affector for affecting direction of particle
class CParticleGravityAffector : public IParticleAffector
{
public:

	CParticleGravityAffector(
		ox::core::CVector3d<float> gravity = ox::core::CVector3d<float>(0.0f,-0.03f,0.0f),
		unsigned int timeForceLost = 1000);

	//! Affects a particle.
	virtual void affect(unsigned int now, SParticle* particlearray, unsigned int count);

private:

	float TimeForceLost;
	ox::core::CVector3d<float> Gravity;
};

} // end namespace scene
} // end namespace daisy


#endif

