// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CParticleFadeOutAffector.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "CParticleFadeOutAffector.h"
#include "ox/video/SColor.h"
#include "daisy/os.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CParticleFadeOutAffector::CParticleFadeOutAffector(
	ox::video::SColor targetColor, unsigned int fadeOutTime)
	: TargetColor(targetColor)
{
	FadeOutTime = fadeOutTime ? (float)fadeOutTime : 1.0f;
}


//! Affects an array of particles.
void CParticleFadeOutAffector::affect(unsigned int now, SParticle* particlearray, unsigned int count)
{
	float d;

	for (unsigned int i=0; i<count; ++i)
		if (particlearray[i].endTime - now < FadeOutTime)
		{
			d = (particlearray[i].endTime - now) / FadeOutTime;
			particlearray[i].color = particlearray[i].startColor.getInterpolated(
				TargetColor, d);
		}
}


} // end namespace scene
} // end namespace daisy

