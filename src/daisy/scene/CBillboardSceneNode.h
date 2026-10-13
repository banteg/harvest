// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CBillboardSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_BILLBOARD_SCENE_NODE_H_INCLUDED__
#define __C_BILLBOARD_SCENE_NODE_H_INCLUDED__

#include "ox/scene/IBillboardSceneNode.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"
#include "ox/video/S3DVertex.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! Scene node which is a billboard. ox::core::A billboard is like a 3d sprite: ox::core::A 2d element,
//! which always looks to the camera. 
class CBillboardSceneNode : public IBillboardSceneNode
{
public:

	//! constructor
	CBillboardSceneNode(ISceneNode* parent, ISceneManager* mgr, int id,	
		const ox::core::CVector3d<float>& position, const ox::core::CDimension2d<float>& size);

	virtual ~CBillboardSceneNode();

	//! pre render event
	virtual void OnPreRender();

	//! render
	virtual void render();

	//! returns the axis aligned bounding box of this node
	virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

	//! sets the size of the billboard
	virtual void setSize(const ox::core::CDimension2d<float>& size);

	//! gets the size of the billboard
	virtual const ox::core::CDimension2d<float>& getSize();

	virtual ox::video::SMaterial& getMaterial(int i);
	
	//! returns amount of materials used by this scene node.
	virtual int getMaterialCount();
	
private:

	ox::core::CDimension2d<float> Size;
	ox::core::CAabbox3d<float> BBox;
	ox::video::SMaterial Material;

	ox::video::S3DVertex vertices[4];
	unsigned short indices[6];
};


} // end namespace scene
} // end namespace daisy


#endif

