// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CTriangleSelector.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CLine3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CTriangle3d.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/S3DVertex2TCoords.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/IMeshBuffer.h"
#include "CTriangleSelector.h"
#include "ox/scene/ISceneNode.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CTriangleSelector::CTriangleSelector(ISceneNode* node)
: SceneNode(node)
{
}


//! constructor
CTriangleSelector::CTriangleSelector(IMesh* mesh, ISceneNode* node)
: SceneNode(node)
{

	int cnt = mesh->getMeshBufferCount();
	for (int i=0; i<cnt; ++i)
	{
		IMeshBuffer* buf = mesh->getMeshBuffer(i);

		int idxCnt = buf->getIndexCount();
		int vtxCnt = buf->getVertexCount();
		const unsigned short* indices = buf->getIndices();
		ox::core::CTriangle3d<float> tri;

		switch ((int)buf->getVertexType())
		{
		case ox::video::EVT_STANDARD:
			{
				ox::video::S3DVertex* vtx = (ox::video::S3DVertex*)buf->getVertices();
				for (int i=0; i<idxCnt; i+=3)
				{
					tri.pointA = vtx[indices[i+0]].Pos;
					tri.pointB = vtx[indices[i+1]].Pos;
					tri.pointC = vtx[indices[i+2]].Pos;
					Triangles.push_back(tri);
				}
			}
			break;
		case ox::video::EVT_2TCOORDS:
			{
				ox::video::S3DVertex2TCoords* vtx = (ox::video::S3DVertex2TCoords*)buf->getVertices();
				for (int i=0; i<idxCnt; i+=3)
				{
					tri.pointA = vtx[indices[i+0]].Pos;
					tri.pointB = vtx[indices[i+1]].Pos;
					tri.pointC = vtx[indices[i+2]].Pos;
					Triangles.push_back(tri);
				}
			}
			break;
		}
	}
}


//! constructor
CTriangleSelector::CTriangleSelector(ox::core::CAabbox3d<float> box, ISceneNode* node)
: SceneNode(node)
{

	// TODO
}



//! destructor
CTriangleSelector::~CTriangleSelector()
{
}



//! Gets all triangles.
void CTriangleSelector::getTriangles(ox::core::CTriangle3d<float>* triangles,
									 int arraySize, int& outTriangleCount,
									const ox::core::CMatrix4* transform)
{
	int cnt = Triangles.size();
	if (cnt > arraySize)
		cnt = arraySize;

	ox::core::CMatrix4 mat;

	if (transform)
		mat = (*transform);

	if (SceneNode)
		mat *= SceneNode->getAbsoluteTransformation();

	for (int i=0; i<cnt; ++i)
	{
		triangles[i] = Triangles[i];
		mat.transformVect(triangles[i].pointA);
		mat.transformVect(triangles[i].pointB);
		mat.transformVect(triangles[i].pointC);
	}

	outTriangleCount = cnt;
}



//! Gets all triangles which lie within a specific bounding box.
void CTriangleSelector::getTriangles(ox::core::CTriangle3d<float>* triangles,
									 int arraySize, int& outTriangleCount,
									const ox::core::CAabbox3d<float>& box,
									const ox::core::CMatrix4* transform)
{
	// return all triangles
	getTriangles(triangles, arraySize, outTriangleCount, transform);
}


//! Gets all triangles which have or may have contact with a 3d line.
void CTriangleSelector::getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize,
	int& outTriangleCount, const ox::core::CLine3d<float>& line,
	const ox::core::CMatrix4* transform)
{
	// return all triangles
	getTriangles(triangles, arraySize, outTriangleCount, transform);
}


//! Returns amount of all available triangles in this selector
int CTriangleSelector::getTriangleCount() const
{
	return Triangles.size();
}



} // end namespace scene
} // end namespace daisy
