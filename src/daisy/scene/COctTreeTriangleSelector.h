// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/COctTreeTriangleSelector.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_OCT_TREE_TRIANGLE_SELECTOR_H_INCLUDED__
#define __C_OCT_TREE_TRIANGLE_SELECTOR_H_INCLUDED__

#include "CTriangleSelector.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CLine3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CTriangle3d.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/ISceneNode.h"
#include "ox/TArray.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;


//! Stupid triangle selector without optimization
class COctTreeTriangleSelector : public CTriangleSelector
{
public:

	//! Constructs a selector based on a mesh
	COctTreeTriangleSelector(IMesh* mesh, ISceneNode* node, int minimalPolysPerNode);

	~COctTreeTriangleSelector();

	//! Gets all triangles which lie within a specific bounding box.
	void getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize, int& outTriangleCount, 
		const ox::core::CAabbox3d<float>& box, const ox::core::CMatrix4* transform=0);

	//! Gets all triangles which have or may have contact with a 3d line.
	virtual void getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize,
		int& outTriangleCount, const ox::core::CLine3d<float>& line, 
		const ox::core::CMatrix4* transform=0);

private:

	struct SOctTreeNode
	{
		SOctTreeNode()
		{
			for (int i=0; i<8; ++i)
				Child[i] = 0;
		}

		~SOctTreeNode()
		{
			for (int i=0; i<8; ++i)
				delete Child[i];
		}

		ox::TArray<ox::core::CTriangle3d<float> > Triangles;
		SOctTreeNode* Child[8];
		ox::core::CAabbox3d<float> Box;
	};


	void constructOctTree(SOctTreeNode* node);
	void deleteEmptyNodes(SOctTreeNode* node);
	void getTrianglesFromOctTree(SOctTreeNode* node, int& trianglesWritten, int maximumSize, 
		const ox::core::CAabbox3d<float>& box, const ox::core::CMatrix4* transform,
		ox::core::CTriangle3d<float>* triangles);

	SOctTreeNode* Root;
	int NodeCount;
	int MinimalPolysPerNode;

};

} // end namespace scene
} // end namespace daisy


#endif

