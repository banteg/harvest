// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneNodeAnimatorDelete.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "CSceneNodeAnimatorDelete.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/ISceneManager.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;


//! constructor
CSceneNodeAnimatorDelete::CSceneNodeAnimatorDelete(ISceneManager* manager, unsigned int time)
: DeleteTime(time), SceneManager(manager)
{
}



//! destructor
CSceneNodeAnimatorDelete::~CSceneNodeAnimatorDelete()
{
}



//! animates a scene node
void CSceneNodeAnimatorDelete::animateNode(ISceneNode* node, unsigned int timeMs)
{
	if (timeMs > DeleteTime && node && SceneManager)
		SceneManager->addToDeletionQueue(node);
}



} // end namespace scene
} // end namespace daisy
