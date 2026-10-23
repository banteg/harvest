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
	float d;

	for (unsigned int i=0; i<count; ++i)
	{
		d = (now - particlearray[i].startTime) / TimeForceLost;
		if (d > 1.0f) d = 1.0f;
		if (d < 0.0f) d = 0.0f;
		d = 1.0f - d;

		particlearray[i].vector = ox::core::CVector3d<float>((Gravity).X*(1.0f - (d)) + particlearray[i].startVector.X*(d), (Gravity).Y*(1.0f - (d)) + particlearray[i].startVector.Y*(d), (Gravity).Z*(1.0f - (d)) + particlearray[i].startVector.Z*(d));
	}
}


} // end namespace scene
} // end namespace daisy
