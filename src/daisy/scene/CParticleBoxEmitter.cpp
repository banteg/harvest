// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CParticleBoxEmitter.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CVector3d.h"
#include "ox/core/irrMath.h"
#include "ox/video/SColor.h"
#include "ox/scene/SParticle.h"
#include "ox/algo/CRand.h"
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
	Time += timeSinceLastCall;

	unsigned int pps = (MaxParticlesPerSecond - MinParticlesPerSecond);
	float perSecond = pps ? (float)MinParticlesPerSecond + (ox::algo::CRand::rand() % pps) : MinParticlesPerSecond;
	float everyWhatMillisecond = 1000.0f / perSecond;

	if (Time > everyWhatMillisecond)
	{
		Particles.clear();
		int amount = (int)((Time / everyWhatMillisecond) + 0.5f);
        Time = 0;
		SParticle p;
		ox::core::CVector3d<float> extend = ((Box).MaxEdge - (Box).MinEdge);

		if (amount > (int)MaxParticlesPerSecond*2)
			amount = MaxParticlesPerSecond * 2;

		double len = Direction.getLength();

		for (int i=0; i<amount; ++i)
		{
			p.pos.X = Box.MinEdge.X + fmodf((float)ox::algo::CRand::rand(), extend.X);
			p.pos.Y = Box.MinEdge.Y + fmodf((float)ox::algo::CRand::rand(), extend.Y);
			p.pos.Z = Box.MinEdge.Z + fmodf((float)ox::algo::CRand::rand(), extend.Z);

			p.startTime = now;
			p.vector = Direction;

			if (MaxAngleDegrees)
			{
				ox::core::CVector3d<float> tgt = Direction;
				{ double deg = ((ox::algo::CRand::rand()%(MaxAngleDegrees*2)) - MaxAngleDegrees) * 0.017453290522098541; float cs = (float)cos(deg); float sn = (float)sin(deg); tgt.X -= (ox::core::CVector3d<float>(0,0,0)).X; tgt.Y -= (ox::core::CVector3d<float>(0,0,0)).Y; tgt.set(tgt.X*cs - tgt.Y*sn, tgt.X*sn + tgt.Y*cs, tgt.Z); tgt.X += (ox::core::CVector3d<float>(0,0,0)).X; tgt.Y += (ox::core::CVector3d<float>(0,0,0)).Y; }
				{ double deg = ((ox::algo::CRand::rand()%(MaxAngleDegrees*2)) - MaxAngleDegrees) * 0.017453290522098541; float cs = (float)cos(deg); float sn = (float)sin(deg); tgt.Z -= (ox::core::CVector3d<float>(0,0,0)).Z; tgt.Y -= (ox::core::CVector3d<float>(0,0,0)).Y; tgt.set(tgt.X, tgt.Y*cs - tgt.Z*sn, tgt.Y*sn + tgt.Z*cs); tgt.Z += (ox::core::CVector3d<float>(0,0,0)).Z; tgt.Y += (ox::core::CVector3d<float>(0,0,0)).Y; }
				p.vector = tgt;
			}

			if (MaxLifeTime - MinLifeTime == 0)
				p.endTime = now + MinLifeTime;
			else
				p.endTime = now + MinLifeTime + (ox::algo::CRand::rand() % (MaxLifeTime - MinLifeTime));

			p.color = MinStartColor.getInterpolated(
				MaxStartColor, (ox::algo::CRand::rand() % 100) / 100.0f);

			p.startColor = p.color;
			p.startVector = p.vector;

			Particles.push_back(p);
		}

		outArray = &Particles[0];

		return Particles.size();
	}

	return 0;
}

} // end namespace scene
} // end namespace daisy
