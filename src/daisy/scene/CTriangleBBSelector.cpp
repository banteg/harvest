// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CTriangleBBSelector.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CTriangle3d.h"
#include "ox/core/CVector3d.h"
#include "CTriangleBBSelector.h"
#include "ox/scene/ISceneNode.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CTriangleBBSelector::CTriangleBBSelector(ISceneNode* node)
: CTriangleSelector(node)
{

	// a box has 12 triangles.
#ifdef HARVEST_PORT
	Triangles.resize(12);
#else
	Triangles.reserve(12);
#endif
}



//! Gets all triangles.
void CTriangleBBSelector::getTriangles(ox::core::CTriangle3d<float>* triangles,
									 int arraySize, int& outTriangleCount,
									const ox::core::CMatrix4* transform)
{
	if (!SceneNode)
		return;

	// construct triangles
	ox::core::CAabbox3d<float> box = SceneNode->getBoundingBox();
	ox::core::CVector3d<float> edges[8];
	box.getEdges(edges);

	Triangles[0].set( edges[3], edges[0], edges[2]);
	Triangles[1].set( edges[3], edges[1], edges[0]);

	Triangles[2].set( edges[3], edges[2], edges[7]);
	Triangles[3].set( edges[7], edges[2], edges[6]);

	Triangles[4].set( edges[7], edges[6], edges[4]);
	Triangles[5].set( edges[5], edges[7], edges[4]);

	Triangles[6].set( edges[5], edges[4], edges[0]);
	Triangles[7].set( edges[5], edges[0], edges[1]);

	Triangles[8].set( edges[1], edges[3], edges[7]);
	Triangles[9].set( edges[1], edges[7], edges[5]);

	Triangles[10].set(edges[0], edges[6], edges[2]);
	Triangles[11].set(edges[0], edges[4], edges[6]);

	// call parent
	CTriangleSelector::getTriangles(triangles, arraySize, outTriangleCount,	transform);
}


} // end namespace scene
} // end namespace daisy
