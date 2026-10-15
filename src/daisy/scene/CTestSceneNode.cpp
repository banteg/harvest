// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CTestSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SColor.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/ISceneNode.h"
#include "CTestSceneNode.h"
#include "ox/video/IVideoDriver.h"
#include "ox/scene/ISceneManager.h"
#include "ox/video/S3DVertex.h"
#include "daisy/os.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CTestSceneNode::CTestSceneNode(float size, ISceneNode* parent, ISceneManager* mgr, int id,
			const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& rotation,	const ox::core::CVector3d<float>& scale)
: ISceneNode(parent, mgr, id, position, rotation, scale)
{

	unsigned short u[36] = {   0,2,1,   0,3,2,   1,5,4,   1,2,5,   4,6,7,   4,5,6,
            7,3,0,   7,6,3,   9,5,2,   9,8,5,   0,11,10,   0,10,7};

	int i; // new ISO for scoping problem with different compilers
	for (i=0; i<36; ++i)
		Indices[i] = u[i];

	Material.Wireframe = false;
	Material.Lighting = false;

	// nicer texture mapping sent in by Dr Andros C Bragianos
	// .. and then improved by jox.
	Vertices[0]  = ox::video::S3DVertex(0,0,0, -1,-1,-1, ox::video::SColor(255,255,255,255), 0, 1);
	Vertices[1]  = ox::video::S3DVertex(1,0,0,  1,-1,-1, ox::video::SColor(255,255,255,255), 1, 1);
	Vertices[2]  = ox::video::S3DVertex(1,1,0,  1, 1,-1, ox::video::SColor(255,255,255,255), 1, 0);
	Vertices[3]  = ox::video::S3DVertex(0,1,0, -1, 1,-1, ox::video::SColor(255,255,255,255), 0, 0);
	Vertices[4]  = ox::video::S3DVertex(1,0,1,  1,-1, 1, ox::video::SColor(255,255,255,255), 0, 1);
	Vertices[5]  = ox::video::S3DVertex(1,1,1,  1, 1, 1, ox::video::SColor(255,255,255,255), 0, 0);
	Vertices[6]  = ox::video::S3DVertex(0,1,1, -1, 1, 1, ox::video::SColor(255,255,255,255), 1, 0);
	Vertices[7]  = ox::video::S3DVertex(0,0,1, -1,-1, 1, ox::video::SColor(255,255,255,255), 1, 1);
	Vertices[8]  = ox::video::S3DVertex(0,1,1, -1, 1, 1, ox::video::SColor(255,255,255,255), 0, 1);
	Vertices[9]  = ox::video::S3DVertex(0,1,0, -1, 1,-1, ox::video::SColor(255,255,255,255), 1, 1);
	Vertices[10] = ox::video::S3DVertex(1,0,1,  1,-1, 1, ox::video::SColor(255,255,255,255), 1, 0);
	Vertices[11] = ox::video::S3DVertex(1,0,0,  1,-1,-1, ox::video::SColor(255,255,255,255), 0, 0);

	Box.reset(0,0,0);

	for (i=0; i<12; ++i)
	{
		Vertices[i].Pos -= ox::core::CVector3d<float>(0.5f, 0.5f, 0.5f);
		Vertices[i].Pos *= size;
		Box.addInternalPoint(Vertices[i].Pos);
	}
}



//! destructor
CTestSceneNode::~CTestSceneNode()
{
}


//! renders the node.
void CTestSceneNode::render()
{
	/*
        011         111
          /6--------/5        y
         /  |      / |        ^  z
        /   |     /  |        | /
    010 3---------2  |        |/
        |   7- - -| -4 101     *---->x
        |  /      |  /
        |/        | /
        0---------1/
       000       100
	*/

	ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();

	driver->setMaterial(Material);

	driver->setTransform(ox::video::ETS_WORLD, AbsoluteTransformation);

	  driver->drawIndexedTriangleList(&Vertices[0], 12, &Indices[0], 12);
}



//! returns the axis aligned bounding box of this node
const ox::core::CAabbox3d<float>& CTestSceneNode::getBoundingBox() const
{
	return Box;
}


void CTestSceneNode::OnPreRender()
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


//! returns the material based on the zero based index i. To get the amount
//! of materials used by this scene node, use getMaterialCount().
//! This function is needed for inserting the node into the scene hirachy on a
//! optimal position for minimizing renderstate changes, but can also be used
//! to directly modify the material of a scene node.
ox::video::SMaterial& CTestSceneNode::getMaterial(int i)
{
	return Material;
}


//! returns amount of materials used by this scene node.
int CTestSceneNode::getMaterialCount()
{
	return 1;
}




} // end namespace scene
} // end namespace daisy
