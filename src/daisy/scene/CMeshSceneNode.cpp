// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CMeshSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SColor.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/IMeshBuffer.h"
#include "ox/scene/ISceneNode.h"
#include "CMeshSceneNode.h"
#include "ox/video/IVideoDriver.h"
#include "ox/scene/ISceneManager.h"
#include "ox/video/S3DVertex.h"
#include "ox/scene/SViewFrustrum.h"
#include "ox/scene/ICameraSceneNode.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;



//! destructor
CMeshSceneNode::~CMeshSceneNode()
{
	if (Mesh)
		Mesh->drop();
}

//! constructor
CMeshSceneNode::CMeshSceneNode(IMesh* mesh, ISceneNode* parent, ISceneManager* mgr, int id,
			const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& rotation,	const ox::core::CVector3d<float>& scale)
: ISceneNode(parent, mgr, id, position, rotation, scale), Mesh(mesh)
{
	if (Mesh)
	{
		// get materials.
		ox::video::SMaterial mat;
		for (int i=0; i<Mesh->getMeshBufferCount(); ++i)
		{
			IMeshBuffer* mb = Mesh->getMeshBuffer(i);
			if (mb)
				mat = mb->getMaterial();

			Materials.push_back(mat);
		}

		// grab the mesh

		Mesh->grab();
	}
}







//! frame
void CMeshSceneNode::OnPreRender()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, SNRT_DEFAULT);

	ISceneNode::OnPreRender();
}



//! renders the node.
void CMeshSceneNode::render()
{
	ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();

	if (!Mesh || !driver)
		return;

	driver->setTransform(ox::video::ETS_WORLD, AbsoluteTransformation);
	Box = Mesh->getBoundingBox();

	// for debug purposes only:
	if (DebugDataVisible)
	{
		ox::video::SMaterial m;
		m.Lighting = false;
		driver->setMaterial(m);
		driver->draw3DBox(Box, ox::video::SColor(0,255,255,255));
	}

	for (int i=0; i<Mesh->getMeshBufferCount(); ++i)
	{
		ox::scene::IMeshBuffer* mb = Mesh->getMeshBuffer(i);

		driver->setMaterial(Materials[i]);
		driver->drawMeshBuffer(mb);
	}
}


//! returns the axis aligned bounding box of this node
const ox::core::CAabbox3d<float>& CMeshSceneNode::getBoundingBox() const
{
	return Mesh ? Mesh->getBoundingBox() : Box;
}


//! returns the material based on the zero based index i. To get the amount
//! of materials used by this scene node, use getMaterialCount().
//! This function is needed for inserting the node into the scene hirachy on a
//! optimal position for minimizing renderstate changes, but can also be used
//! to directly modify the material of a scene node.
ox::video::SMaterial& CMeshSceneNode::getMaterial(int i)
{
	if (i < 0 || i >= (int)Materials.size())
		return ISceneNode::getMaterial(i);

	return Materials[i];
}



//! returns amount of materials used by this scene node.
int CMeshSceneNode::getMaterialCount()
{
	return Materials.size();
}



} // end namespace scene
} // end namespace daisy
