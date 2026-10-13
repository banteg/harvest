// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CEmptySceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/scene/ISceneNode.h"
#include "CEmptySceneNode.h"
#include "ox/video/IVideoDriver.h"
#include "ox/scene/ISceneManager.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CEmptySceneNode::CEmptySceneNode(ISceneNode* parent, ISceneManager* mgr, int id)
: ISceneNode(parent, mgr, id)
{
	setAutomaticCulling(false);
}


//! pre render event
void CEmptySceneNode::OnPreRender()
{
	if (IsVisible)
	{
		SceneManager->registerNodeForRendering(this, ox::scene::SNRT_DEFAULT);
		ISceneNode::OnPreRender();
	}
}


//! render
void CEmptySceneNode::render()
{
	// do nothing
}


//! returns the axis aligned bounding box of this node
const ox::core::CAabbox3d<float>& CEmptySceneNode::getBoundingBox() const
{
	return Box;
}

} // end namespace scene
} // end namespace daisy
