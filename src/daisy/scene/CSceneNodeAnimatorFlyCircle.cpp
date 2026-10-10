// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneNodeAnimatorFlyCircle.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "CSceneNodeAnimatorFlyCircle.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/ISceneNode.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CSceneNodeAnimatorFlyCircle::CSceneNodeAnimatorFlyCircle(unsigned int time, const ox::core::CVector3d<float>& center, float radius, float speed)
: Radius(radius), Center(center), Speed(speed), StartTime(time)
{
}



//! destructor
CSceneNodeAnimatorFlyCircle::~CSceneNodeAnimatorFlyCircle()
{
}



//! animates a scene node
void CSceneNodeAnimatorFlyCircle::animateNode(ISceneNode* node, unsigned int timeMs)
{
	ox::core::CMatrix4 mat;

	float t = (timeMs-StartTime) * Speed;

	ox::core::CVector3d<float> circle(Radius * (float)sin(t), 0, Radius * (float)cos(t));
	node->setPosition(Center + circle);
}


} // end namespace scene
} // end namespace daisy

