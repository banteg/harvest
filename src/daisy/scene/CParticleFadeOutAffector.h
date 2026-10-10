// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CParticleFadeOutAffector.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_PARTICLE_FADE_OUT_AFFECTOR_H_INCLUDED__
#define __C_PARTICLE_FADE_OUT_AFFECTOR_H_INCLUDED__

#include "IParticleAffector.h"
#include "ox/video/SColor.h"
#include "ox/video/ColorPacking.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! Particle Affector for fading out a color
class CParticleFadeOutAffector : public IParticleAffector
{
public:

	CParticleFadeOutAffector(ox::video::SColor targetColor, unsigned int fadeOutTime);

	//! Affects a particle.
	virtual void affect(unsigned int now, SParticle* particlearray, unsigned int count);

private:

	ox::video::SColor TargetColor;
	float FadeOutTime;
};

} // end namespace scene
} // end namespace daisy


#endif

