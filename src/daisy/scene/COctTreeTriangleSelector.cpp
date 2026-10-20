// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/COctTreeTriangleSelector.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CLine3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CTriangle3d.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/IMesh.h"
#include "ox/event/ILogger.h"
#include "COctTreeTriangleSelector.h"
#include "ox/scene/ISceneNode.h"

#include "daisy/os.h"
#include <stdio.h>

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
COctTreeTriangleSelector::COctTreeTriangleSelector(IMesh* mesh, ISceneNode* node,
												   int minimalPolysPerNode)
: CTriangleSelector(mesh, node), Root(0), NodeCount(0),
 MinimalPolysPerNode(minimalPolysPerNode)
{


	if (!Triangles.empty())
	{
		unsigned int start = os::Timer::getTime();

		// create the triangle octtree
		Root = new SOctTreeNode();
		Root->Triangles = Triangles;
		constructOctTree(Root);

		unsigned int end = os::Timer::getTime();
		char tmp[255];
		sprintf(tmp, "Needed %dms to create OctTreeTriangleSelector.(%d nodes, %d polys)",
			end - start, NodeCount, Triangles.size());
		os::Printer::log(tmp, ox::event::ELL_INFORMATION);
	}
}



//! destructor
COctTreeTriangleSelector::~COctTreeTriangleSelector()
{
	delete Root;
}



void COctTreeTriangleSelector::constructOctTree(SOctTreeNode* node)
{
	++NodeCount;

	node->Box.reset(node->Triangles[0].pointA);

	// get bounding box
	int cnt = node->Triangles.size();
	for (int i=0; i<cnt; ++i)
	{
		node->Box.addInternalPoint(node->Triangles[i].pointA);
		node->Box.addInternalPoint(node->Triangles[i].pointB);
		node->Box.addInternalPoint(node->Triangles[i].pointC);
	}

	ox::core::CVector3d<float> middle = node->Box.getCenter();
	ox::core::CVector3d<float> edges[8];
	node->Box.getEdges(edges);

	ox::core::CAabbox3d<float> box;

	// calculate children

	if (!node->Box.isEmpty() && (int)node->Triangles.size() > MinimalPolysPerNode)
	for (int ch=0; ch<8; ++ch)
	{
		box.reset(middle);
		box.addInternalPoint(edges[ch]);
		node->Child[ch] = new SOctTreeNode();

		for (int i=0; i<(int)node->Triangles.size(); ++i)
		{
			if (node->Triangles[i].isTotalInsideBox(box))
			{
				node->Child[ch]->Triangles.push_back(node->Triangles[i]);
				node->Triangles.erase(node->Triangles.begin() + i);
				--i;
			}
		}

		if (node->Child[ch]->Triangles.empty())
		{
			delete node->Child[ch];
			node->Child[ch] = 0;
		}
		else
			constructOctTree(node->Child[ch]);
	}
}



//! Gets all triangles which lie within a specific bounding box.
void COctTreeTriangleSelector::getTriangles(ox::core::CTriangle3d<float>* triangles,
									 int arraySize, int& outTriangleCount,
									const ox::core::CAabbox3d<float>& box,
									const ox::core::CMatrix4* transform)
{
	ox::core::CMatrix4 mat;

	if (SceneNode)
		mat = SceneNode->getAbsoluteTransformation();

	mat.makeInverse();
	ox::core::CAabbox3d<float> invbox = box;
	mat.transformBox(invbox);

	mat.makeIdentity();

	if (transform)
		mat = (*transform);

	if (SceneNode)
		mat *= SceneNode->getAbsoluteTransformation();

	int trianglesWritten = 0;

	if (Root)
		getTrianglesFromOctTree(Root, trianglesWritten,
			arraySize, invbox, &mat, triangles);

	outTriangleCount = trianglesWritten;
}


void COctTreeTriangleSelector::getTrianglesFromOctTree(
		SOctTreeNode* node, int& trianglesWritten,
		int maximumSize, const ox::core::CAabbox3d<float>& box,
		const ox::core::CMatrix4* mat, ox::core::CTriangle3d<float>* triangles)
{
	if (!box.intersectsWithBox(node->Box))
		return;

	int cnt = node->Triangles.size();
	if (cnt + trianglesWritten > maximumSize)
		cnt -= cnt + trianglesWritten - maximumSize;

	int i;

	for (i=0; i<cnt; ++i)
	{
		triangles[trianglesWritten] = node->Triangles[i];
		mat->transformVect(triangles[trianglesWritten].pointA);
		mat->transformVect(triangles[trianglesWritten].pointB);
		mat->transformVect(triangles[trianglesWritten].pointC);
		++trianglesWritten;
	}

	for (i=0; i<8; ++i)
		if (node->Child[i])
			getTrianglesFromOctTree(node->Child[i], trianglesWritten,
			maximumSize, box, mat, triangles);
}


//! Gets all triangles which have or may have contact with a 3d line.
void COctTreeTriangleSelector::getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize,
	int& outTriangleCount, const ox::core::CLine3d<float>& line,
	const ox::core::CMatrix4* transform)
{
	ox::core::CAabbox3d<float> box(line.start);
	box.addInternalPoint(line.end);

	// TODO: Could be optimized for line a little bit more.
	COctTreeTriangleSelector::getTriangles(triangles, arraySize, outTriangleCount,
		box, transform);
}


} // end namespace scene
} // end namespace daisy
