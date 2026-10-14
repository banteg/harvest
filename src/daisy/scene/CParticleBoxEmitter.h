// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CParticleBoxEmitter.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_PARTICLE_BOX_EMITTER_H_INCLUDED__
#define __C_PARTICLE_BOX_EMITTER_H_INCLUDED__

#include "ox/scene/IParticleEmitter.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SColor.h"
#include "ox/scene/SParticle.h"
#include "ox/TArray.h"
#include "ox/core/CAabbox3d.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! ox::core::A default box emitter
class CParticleBoxEmitter : public IParticleEmitter
{
public:

	//! constructor
	CParticleBoxEmitter(
		ox::core::CAabbox3d<float> box,
		ox::core::CVector3d<float> direction = ox::core::CVector3d<float>(0.0f,0.03f,0.0f), 
		unsigned int minParticlesPerSecond = 20,
		unsigned int maxParticlePerSecond = 40,
		ox::video::SColor minStartColor = ox::video::SColor(255,0,0,0),
		ox::video::SColor maxStartColor = ox::video::SColor(255,255,255,255),
		unsigned int lifeTimeMin=2000,
		unsigned int lifeTimeMax=4000,
		int maxAngleDegrees=0);

	//! Prepares an array with new particles to emitt into the system
	//! and returns how much new particles there are.
	virtual int emitt(unsigned int now, unsigned int timeSinceLastCall, SParticle*& outArray);

private:

	ox::TArray<SParticle> Particles;
	ox::core::CAabbox3d<float> Box;
	ox::core::CVector3d<float> Direction;
	unsigned int MinParticlesPerSecond, MaxParticlesPerSecond;
	ox::video::SColor MinStartColor, MaxStartColor;
	unsigned int MinLifeTime, MaxLifeTime;

	unsigned int Time;
	unsigned int Emitted;
	int MaxAngleDegrees;
};

} // end namespace scene
} // end namespace daisy


#endif

