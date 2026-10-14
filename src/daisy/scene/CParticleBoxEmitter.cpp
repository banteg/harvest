// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CParticleBoxEmitter.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SColor.h"
#include "ox/scene/SParticle.h"
#include "CParticleBoxEmitter.h"
#include "daisy/os.h"
#include <math.h>

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CParticleBoxEmitter::CParticleBoxEmitter(
	ox::core::CAabbox3d<float> box,
	ox::core::CVector3d<float> direction, 	unsigned int minParticlesPerSecond,
	unsigned int maxParticlePerSecond,	ox::video::SColor minStartColor,
	ox::video::SColor maxStartColor, unsigned int lifeTimeMin, unsigned int lifeTimeMax,
	int maxAngleDegrees)
 : Box(box), Direction(direction), MinParticlesPerSecond(minParticlesPerSecond),
	MaxParticlesPerSecond(maxParticlePerSecond), 
	MinStartColor(minStartColor), MaxStartColor(maxStartColor),
	MinLifeTime(lifeTimeMin), MaxLifeTime(lifeTimeMax), Time(0), Emitted(0),
	MaxAngleDegrees(maxAngleDegrees)
{

}



//! Prepares an array with new particles to emitt into the system
//! and returns how much new particles there are.
int CParticleBoxEmitter::emitt(unsigned int now, unsigned int timeSinceLastCall, SParticle*& outArray)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}

} // end namespace scene
} // end namespace daisy

