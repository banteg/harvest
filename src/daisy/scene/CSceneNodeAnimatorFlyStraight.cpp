// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneNodeAnimatorFlyStraight.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "CSceneNodeAnimatorFlyStraight.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/ISceneNode.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;


//! constructor
CSceneNodeAnimatorFlyStraight::CSceneNodeAnimatorFlyStraight(const ox::core::CVector3d<float>& startPoint, 
							const ox::core::CVector3d<float>& endPoint, unsigned int timeForWay,
							bool loop, unsigned int now)
: Start(startPoint), End(endPoint), StartTime(now), TimeForWay(timeForWay), Loop(loop)
{

	EndTime = now + timeForWay;

	Vector = End - Start;
	WayLength = (float)Vector.getLength();
	Vector.normalize();

	TimeFactor = WayLength / TimeForWay;
}



//! destructor
CSceneNodeAnimatorFlyStraight::~CSceneNodeAnimatorFlyStraight()
{
}



//! animates a scene node
void CSceneNodeAnimatorFlyStraight::animateNode(ISceneNode* node, unsigned int timeMs)
{
	if (!node)
		return;

	unsigned int t = (timeMs-StartTime);

	ox::core::CVector3d<float> pos = Start;

	if (!Loop && t >= TimeForWay)
		pos = End;
	else
		pos += Vector * (float)fmod((float)t, (float)TimeForWay) * TimeFactor;

	node->setPosition(pos);
}



} // end namespace scene
} // end namespace daisy
