// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CParticleGravityAffector.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CVector3d.h"
#include "ox/scene/SParticle.h"
#include "CParticleGravityAffector.h"
#include "daisy/os.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CParticleGravityAffector::CParticleGravityAffector(
	ox::core::CVector3d<float> gravity,
		unsigned int timeForceLost)
	: Gravity(gravity), TimeForceLost((float)timeForceLost)
{
}


//! Affects an array of particles.
void CParticleGravityAffector::affect(unsigned int now, SParticle* particlearray, unsigned int count)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


} // end namespace scene
} // end namespace daisy

