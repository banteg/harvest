// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CParticlePointEmitter.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_PARTICLE_POINT_EMITTER_H_INCLUDED__
#define __C_PARTICLE_POINT_EMITTER_H_INCLUDED__

#include "ox/scene/IParticleEmitter.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SColor.h"
#include "ox/scene/SParticle.h"
#include "ox/TArray.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! ox::core::A default point emitter
class CParticlePointEmitter : public IParticleEmitter
{
public:

	//! constructor
	CParticlePointEmitter(
		ox::core::CVector3d<float> direction = ox::core::CVector3d<float>(0.0f,0.03f,0.0f), 
		unsigned int minParticlesPerSecond = 5,
		unsigned int maxParticlePerSecond = 10,
		ox::video::SColor minStartColor = ox::video::SColor(255,0,0,0),
		ox::video::SColor maxStartColor = ox::video::SColor(255,255,255,255),
		unsigned int lifeTimeMin=2000,
		unsigned int lifeTimeMax=4000,
		int maxAngleDegrees=0);

	//! Prepares an array with new particles to emitt into the system
	//! and returns how much new particles there are.
	virtual int emitt(unsigned int now, unsigned int timeSinceLastCall, SParticle*& outArray);

private:

	SParticle Particle;
	ox::core::CVector3d<float> Direction;
	unsigned int MinParticlesPerSecond, MaxParticlesPerSecond;
	ox::video::SColor MinStartColor, MaxStartColor;
	unsigned int MinLifeTime, MaxLifeTime;
	int MaxAngleDegrees;

	unsigned int Time;
	unsigned int Emitted;
};

} // end namespace scene
} // end namespace daisy


#endif

