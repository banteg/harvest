// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CLightSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SLight.h"
#include "ox/scene/ILightSceneNode.h"
#include "ox/scene/ISceneNode.h"
#include "CLightSceneNode.h"
#include "ox/video/IVideoDriver.h"
#include "ox/scene/ISceneManager.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CLightSceneNode::CLightSceneNode(ISceneNode* parent, ISceneManager* mgr, int id,	
	const ox::core::CVector3d<float>& position, ox::video::SColorf color,float radius)
: ILightSceneNode(parent, mgr, id, position)
{
	LightData.Radius = radius;
	LightData.DiffuseColor = color;
	LightData.Position = position;
}

CLightSceneNode::~CLightSceneNode()
{
}


//! pre render event
void CLightSceneNode::OnPreRender()
{
	if (IsVisible)
	{
		SceneManager->registerNodeForRendering(this, SNRT_LIGHT_AND_CAMERA);
		ISceneNode::OnPreRender();
	}
}


//! render
void CLightSceneNode::render()
{
	ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();
	if (!driver)
		return;

	LightData.Position = getAbsolutePosition();
	driver->addDynamicLight(LightData);
}


//! returns the light data
ox::video::SLight& CLightSceneNode::getLightData()
{
	return LightData;
}

//! returns the axis aligned bounding box of this node
const ox::core::CAabbox3d<float>& CLightSceneNode::getBoundingBox() const
{
	return BBox;
}

} // end namespace scene
} // end namespace daisy
