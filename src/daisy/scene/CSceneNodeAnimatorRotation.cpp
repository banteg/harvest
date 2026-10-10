// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneNodeAnimatorRotation.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "CSceneNodeAnimatorRotation.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/ISceneNode.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CSceneNodeAnimatorRotation::CSceneNodeAnimatorRotation(unsigned int time, const ox::core::CVector3d<float>& rotation)
: Rotation(rotation), StartTime(time)
{
}


//! destructor
CSceneNodeAnimatorRotation::~CSceneNodeAnimatorRotation()
{
}



//! animates a scene node
void CSceneNodeAnimatorRotation::animateNode(ISceneNode* node, unsigned int timeMs)
{
	if (node) // thanks to warui for this fix
	{ 
		ox::core::CVector3d<float> NewRotation = node->getRotation(); 
		NewRotation += Rotation* ((timeMs-StartTime)/10.0f); 
		node->setRotation(NewRotation); 
		StartTime=timeMs; 
	}
}


} // end namespace scene
} // end namespace daisy

