// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CLightSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_LIGHT_SCENE_NODE_H_INCLUDED__
#define __C_LIGHT_SCENE_NODE_H_INCLUDED__

#include "ox/scene/ILightSceneNode.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SLight.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! Scene node which is a dynamic light. You can switch the light on and off by 
//! making it visible or not, and let it be animated by ordinary scene node animators.
class CLightSceneNode : public ILightSceneNode
{
public:

	//! constructor
	CLightSceneNode(ISceneNode* parent, ISceneManager* mgr, int id,	
		const ox::core::CVector3d<float>& position, ox::video::SColorf color,float range);

	virtual ~CLightSceneNode();

	//! pre render event
	virtual void OnPreRender();

	//! render
	virtual void render();

	//! returns the light data
	virtual ox::video::SLight& getLightData();

	//! returns the axis aligned bounding box of this node
	virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

private:

	ox::video::SLight LightData;
	ox::core::CAabbox3d<float> BBox;
};


} // end namespace scene
} // end namespace daisy


#endif

