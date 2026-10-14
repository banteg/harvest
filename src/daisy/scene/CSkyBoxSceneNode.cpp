// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSkyBoxSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CMatrix4.h"
#include "ox/video/ITexture.h"
#include "ox/video/SColor.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/ISceneNode.h"
#include "CSkyBoxSceneNode.h"
#include "ox/video/IVideoDriver.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/video/S3DVertex.h"
#include "daisy/os.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CSkyBoxSceneNode::CSkyBoxSceneNode(ox::video::ITexture* top, ox::video::ITexture* bottom, ox::video::ITexture* left,
			ox::video::ITexture* right, ox::video::ITexture* front, ox::video::ITexture* back, ISceneNode* parent, ISceneManager* mgr, int id)
: ISceneNode(parent, mgr, id)
{

	// create indices

	Indices[0] = 0;
	Indices[1] = 1; 
	Indices[2] = 2; 
	Indices[3] = 0; 
	Indices[4] = 2; 
	Indices[5] = 3; 

	// create material
	
	ox::video::SMaterial mat;
	mat.Lighting = false;
	mat.ZBuffer = false;
	mat.ZWriteEnable = false;
	mat.BilinearFilter = true;

	/* Hey, I am no artist, but look at that 
	   cool ASCII art I made! ;)

       -111         111
          /6--------/5        y
         /  |      / |        ^  z
        /   |   11-1 |        | /
  -11-1 3---------2  |        |/
        |   7- - -| -4 1-11    *---->x
        | -1-11   |  /       3-------|2
        |/        | /         |    //|
        0---------1/          |  //  |
     -1-1-1     1-1-1         |//    |
	                         0--------1
	*/ 

	float onepixel = 0.0f;

	ox::video::ITexture* tex = front;
	if (!tex) tex = left;
	if (!tex) tex = back;
	if (!tex) tex = right;
	if (!tex) tex = top;
	if (!tex) tex = bottom;

	if (tex)
	{
		ox::core::CDimension2d<int> dim = tex->getSize();
		onepixel = 1.0f / (dim.Width * 1.5f);
	}


	float l = 10.0f;
	float t = 1.0f - onepixel;
	float o = 0.0f + onepixel;

	Material[0] = mat;
	Material[0].Texture1 = front;
	Vertices[0] = ox::video::S3DVertex(-l,-l,-l, 0,0,1, ox::video::SColor(255,255,255,255), o, t);
	Vertices[1] = ox::video::S3DVertex( l,-l,-l, 0,0,1, ox::video::SColor(255,255,255,255), t, t);
	Vertices[2] = ox::video::S3DVertex( l, l,-l, 0,0,1, ox::video::SColor(255,255,255,255), t, o);
	Vertices[3] = ox::video::S3DVertex(-l, l,-l, 0,0,1, ox::video::SColor(255,255,255,255), o, o);

	// create left side

	Material[1] = mat;
	Material[1].Texture1 = left;
	Vertices[4] = ox::video::S3DVertex( l,-l,-l, -1,0,0, ox::video::SColor(255,255,255,255), o, t);
	Vertices[5] = ox::video::S3DVertex( l,-l, l, -1,0,0, ox::video::SColor(255,255,255,255), t, t);
	Vertices[6] = ox::video::S3DVertex( l, l, l, -1,0,0, ox::video::SColor(255,255,255,255), t, o);
	Vertices[7] = ox::video::S3DVertex( l, l,-l, -1,0,0, ox::video::SColor(255,255,255,255), o, o);

	// create back side

	Material[2] = mat;
	Material[2].Texture1 = back;
	Vertices[8]  = ox::video::S3DVertex( l,-l, l, 0,0,-1, ox::video::SColor(255,255,255,255), o, t);
	Vertices[9]  = ox::video::S3DVertex(-l,-l, l, 0,0,-1, ox::video::SColor(255,255,255,255), t, t);
	Vertices[10] = ox::video::S3DVertex(-l, l, l, 0,0,-1, ox::video::SColor(255,255,255,255), t, o);
	Vertices[11] = ox::video::S3DVertex( l, l, l, 0,0,-1, ox::video::SColor(255,255,255,255), o, o);

	// create right side

	Material[3] = mat;
	Material[3].Texture1 = right;
	Vertices[12] = ox::video::S3DVertex(-l,-l, l, 1,0,0, ox::video::SColor(255,255,255,255), o, t);
	Vertices[13] = ox::video::S3DVertex(-l,-l,-l, 1,0,0, ox::video::SColor(255,255,255,255), t, t);
	Vertices[14] = ox::video::S3DVertex(-l, l,-l, 1,0,0, ox::video::SColor(255,255,255,255), t, o);
	Vertices[15] = ox::video::S3DVertex(-l, l, l, 1,0,0, ox::video::SColor(255,255,255,255), o, o);

	// create top side

	Material[4] = mat;
	Material[4].Texture1 = top;
	Vertices[16] = ox::video::S3DVertex( l, l, l, 0,-1,0, ox::video::SColor(255,255,255,255), o, o);
	Vertices[17] = ox::video::S3DVertex(-l, l, l, 0,-1,0, ox::video::SColor(255,255,255,255), o, t);
	Vertices[18] = ox::video::S3DVertex(-l, l,-l, 0,-1,0, ox::video::SColor(255,255,255,255), t, t);
	Vertices[19] = ox::video::S3DVertex( l, l,-l, 0,-1,0, ox::video::SColor(255,255,255,255), t, o);

	// create bottom side

	Material[5] = mat;
	Material[5].Texture1 = bottom;
	Vertices[20] = ox::video::S3DVertex(-l,-l, l, 0,1,0, ox::video::SColor(255,255,255,255), o, o);
	Vertices[21] = ox::video::S3DVertex( l,-l, l, 0,1,0, ox::video::SColor(255,255,255,255), o, t);
	Vertices[22] = ox::video::S3DVertex( l,-l,-l, 0,1,0, ox::video::SColor(255,255,255,255), t, t);
	Vertices[23] = ox::video::S3DVertex(-l,-l,-l, 0,1,0, ox::video::SColor(255,255,255,255), t, o);
}



//! destructor
CSkyBoxSceneNode::~CSkyBoxSceneNode()
{
}


//! renders the node.
void CSkyBoxSceneNode::render()
{
	ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();
	ox::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();

	if (!camera || !driver)
		return;

	ox::core::CMatrix4 mat;
	mat.setTranslation(camera->getAbsolutePosition());

	driver->setTransform(ox::video::ETS_WORLD, mat);
	
	for (int i=0; i<6; ++i)
	{
		driver->setMaterial(Material[i]);
		driver->drawIndexedTriangleList(&Vertices[i*4], 4, Indices, 2);
	}
}



//! returns the axis aligned bounding box of this node
const ox::core::CAabbox3d<float>& CSkyBoxSceneNode::getBoundingBox() const
{
	return Box;
}


void CSkyBoxSceneNode::OnPreRender()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, SNRT_SKY_BOX);

	ISceneNode::OnPreRender();
}


//! returns the material based on the zero based index i. To get the amount
//! of materials used by this scene node, use getMaterialCount().
//! This function is needed for inserting the node into the scene hirachy on a
//! optimal position for minimizing renderstate changes, but can also be used
//! to directly modify the material of a scene node.
ox::video::SMaterial& CSkyBoxSceneNode::getMaterial(int i)
{
	return Material[i];
}


//! returns amount of materials used by this scene node.
int CSkyBoxSceneNode::getMaterialCount()
{
	return 6;
}




} // end namespace scene
} // end namespace daisy

