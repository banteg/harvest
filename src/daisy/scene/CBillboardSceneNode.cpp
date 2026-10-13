// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CBillboardSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IBillboardSceneNode.h"
#include "ox/scene/ISceneNode.h"
#include "CBillboardSceneNode.h"
#include "ox/video/IVideoDriver.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ICameraSceneNode.h"
#include "daisy/os.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CBillboardSceneNode::CBillboardSceneNode(ISceneNode* parent, ISceneManager* mgr, int id,
const ox::core::CVector3d<float>& position, const ox::core::CDimension2d<float>& size)
: IBillboardSceneNode(parent, mgr, id, position)
{
	setSize(size);

	AutomaticCullingEnabled = false;

	indices[0] = 0;
	indices[1] = 2;
	indices[2] = 1;
	indices[3] = 0;
	indices[4] = 3;
	indices[5] = 2;

	vertices[0].TCoords.set(0.0f, 0.0f);
	vertices[0].Color = 0xffffffff;

	vertices[1].TCoords.set(0.0f, 1.0f);
	vertices[1].Color = 0xffffffff;

	vertices[2].TCoords.set(1.0f, 1.0f);
	vertices[2].Color = 0xffffffff;

	vertices[3].TCoords.set(1.0f, 0.0f);
	vertices[3].Color = 0xffffffff;
}



CBillboardSceneNode::~CBillboardSceneNode()
{
}



//! pre render event
void CBillboardSceneNode::OnPreRender()
{
	if (IsVisible)
	{
		SceneManager->registerNodeForRendering(this, ox::scene::SNRT_DEFAULT);
		ISceneNode::OnPreRender();
	}
}


//! render
void CBillboardSceneNode::render()
{
	ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();
	ICameraSceneNode* camera = SceneManager->getActiveCamera();

	if (!camera || !driver)
		return;

	// make billboard look to Camera

	ox::core::CVector3d<float> pos = getAbsolutePosition();

	ox::core::CVector3d<float> campos = camera->getAbsolutePosition();
	ox::core::CVector3d<float> target = camera->getTarget();
	ox::core::CVector3d<float> up = camera->getUpVector();
	ox::core::CVector3d<float> view = target - campos;
	view.normalize();

	ox::core::CVector3d<float> horizontal = up.crossProduct(view);
	horizontal.normalize();

	ox::core::CVector3d<float> vertical = horizontal.crossProduct(view);
	vertical.normalize();

	horizontal *= 0.5f * Size.Width;
	vertical *= 0.5f * Size.Height;

	vertices[0].Pos = pos + horizontal + vertical;
	vertices[1].Pos = pos + horizontal - vertical;
	vertices[2].Pos = pos - horizontal - vertical;
	vertices[3].Pos = pos - horizontal + vertical;

	view *= -1.0f;

	for (int i=0; i<4; ++i)
		vertices[i].Normal = view;

	// draw

	ox::core::CMatrix4 mat;
    driver->setTransform(ox::video::ETS_WORLD, mat);
	driver->setMaterial(Material);

	driver->drawIndexedTriangleList(vertices, 4, indices, 2);
}


//! returns the axis aligned bounding box of this node
const ox::core::CAabbox3d<float>& CBillboardSceneNode::getBoundingBox() const
{
	return BBox;
}


//! sets the size of the billboard
void CBillboardSceneNode::setSize(const ox::core::CDimension2d<float>& size)
{
	Size = size;
}


ox::video::SMaterial& CBillboardSceneNode::getMaterial(int i)
{
	return Material;
}


//! returns amount of materials used by this scene node.
int CBillboardSceneNode::getMaterialCount()
{
	return 1;
}


//! gets the size of the billboard
const ox::core::CDimension2d<float>& CBillboardSceneNode::getSize()
{
	return Size;
}

} // end namespace scene
} // end namespace daisy
