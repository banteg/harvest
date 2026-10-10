// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/SParticle.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __S_PARTICLE_H_INCLUDED__
#define __S_PARTICLE_H_INCLUDED__

#include "ox/core/CVector3d.h"
#include "ox/video/SColor.h"
#include "ox/video/ColorPacking.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;
	//! Struct for holding particle data
	struct SParticle
	{
		//! Position of the particle
		ox::core::CVector3d<float> pos;

		//! Direction and speed of the particle
		ox::core::CVector3d<float> vector;

		//! Start life time of the particle
		unsigned int startTime;

		//! End life time of the particle
		unsigned int endTime;

		//! Current color of the particle
		ox::video::SColor color;

		//! Original color of the particle. That's the color
		//! of the particle it had when it was emitted.
		ox::video::SColor startColor;

		//! Original direction and speed of the particle, 
		//! the direction and speed the particle had when
		//! it was emitted.
		ox::core::CVector3d<float> startVector;
	};


} // end namespace scene
} // end namespace daisy

#endif

