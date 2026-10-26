// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/COctTreeSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/S3DVertex2TCoords.h"
#include "ox/video/SColor.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/IMeshBuffer.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/SViewFrustrum.h"
#include "ox/event/ILogger.h"
#include "ox/TArray.h"
#include "COctTreeSceneNode.h"
#include "OctTree.h"
#include "ox/scene/ISceneManager.h"
#include "ox/video/IVideoDriver.h"
#include "ox/scene/ICameraSceneNode.h"

#include "daisy/os.h"
#include <stdio.h>

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;


//! constructor
COctTreeSceneNode::COctTreeSceneNode(ISceneNode* parent, ISceneManager* mgr,
									 int id, int minimalPolysPerNode)
: ISceneNode(parent, mgr, id), StdOctTree(0), LightMapOctTree(0),
	MinimalPolysPerNode(minimalPolysPerNode)
{

	AutomaticCullingEnabled = false;
	vertexType = (ox::video::E_VERTEX_TYPE)-1;
}



//! destructor
COctTreeSceneNode::~COctTreeSceneNode()
{
	delete StdOctTree;
	delete LightMapOctTree;
}



void COctTreeSceneNode::OnPreRender()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, SNRT_DEFAULT);

	ISceneNode::OnPreRender();
}



//! renders the node.
void COctTreeSceneNode::render()
{
	ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();

	if (vertexType == -1 || !driver)
		return;

	ICameraSceneNode* camera = SceneManager->getActiveCamera();
	if (!camera)
		return;

	driver->setTransform(ox::video::ETS_WORLD, AbsoluteTransformation);

	SViewFrustrum frust = *camera->getViewFrustrum();

	//transform the frustrum to the current absolute transformation

	ox::core::CMatrix4 invTrans(AbsoluteTransformation);
	invTrans.makeInverse();

	frust.transform(invTrans);

    ox::core::CAabbox3d<float> box = frust.getBoundingBox();

	switch((int)vertexType)
	{
	case ox::video::EVT_STANDARD:
		{
			//StdOctTree->calculatePolys(box);
			StdOctTree->calculatePolys(frust);

			OctTree<ox::video::S3DVertex>::SIndexData* d =  StdOctTree->getIndexData();

			for (unsigned int i=0; i<Materials.size(); ++i)
			{
				driver->setMaterial(Materials[i]);
				driver->drawIndexedTriangleList(
					&StdMeshes[i].Vertices[0], StdMeshes[i].Vertices.size(),
					d[i].Indices, d[i].CurrentSize / 3);
			}

			// for debug purposes only
			if (DebugDataVisible && !Materials.empty())
			{
				ox::video::SMaterial m;
				m.Lighting = false;
				ox::TArray< ox::core::CAabbox3d<float> > boxes;
				driver->setMaterial(m);
				StdOctTree->renderBoundingBoxes(box, boxes);
				for (unsigned int b=0; b<boxes.size(); ++b)
					driver->draw3DBox(boxes[b], ox::video::SColor(0,255,255,255));
			}
			break;

		}
	case ox::video::EVT_2TCOORDS:
		{
			//LightMapOctTree->calculatePolys(box);
			LightMapOctTree->calculatePolys(frust);

			OctTree<ox::video::S3DVertex2TCoords>::SIndexData* d =  LightMapOctTree->getIndexData();

			for (unsigned int i=0; i<Materials.size(); ++i)
			{
				driver->setMaterial(Materials[i]);
				driver->drawIndexedTriangleList(
					&LightMapMeshes[i].Vertices[0], LightMapMeshes[i].Vertices.size(),
					d[i].Indices, d[i].CurrentSize / 3);
			}

			// for debug purposes only
			if (DebugDataVisible)
			{
				if (!Materials.empty())
				{
					ox::video::SMaterial m;
					m.Lighting = false;
					ox::TArray< ox::core::CAabbox3d<float> > boxes;
					driver->setMaterial(m);
					LightMapOctTree->renderBoundingBoxes(box, boxes);
					for (unsigned int b=0; b<boxes.size(); ++b)
						driver->draw3DBox(boxes[b], ox::video::SColor(0,255,255,255));
				}
			}
		}
		break;
	};
}


//! returns the axis aligned bounding box of this node
const ox::core::CAabbox3d<float>& COctTreeSceneNode::getBoundingBox() const
{
	return Box;
}


//! creates the tree
bool COctTreeSceneNode::createTree(IMesh* mesh)
{
	if (!mesh)
		return false;

	unsigned int beginTime = os::Timer::getTime();

	int nodeCount = 0;
	int polyCount = 0;

	Box = mesh->getBoundingBox();

	if (mesh->getMeshBufferCount())
	{
		vertexType = mesh->getMeshBuffer(0)->getVertexType();

		switch((int)vertexType)
		{
		case ox::video::EVT_STANDARD:
			{
				for (int i=0; i<mesh->getMeshBufferCount(); ++i)
				{
					IMeshBuffer* b = mesh->getMeshBuffer(i);
					Materials.push_back(b->getMaterial());

					OctTree<ox::video::S3DVertex>::SMeshChunk chunk;
					chunk.MaterialId = i;
					StdMeshes.push_back(chunk);
					OctTree<ox::video::S3DVertex>::SMeshChunk &nchunk = StdMeshes[StdMeshes.size()-1];

					int v;

					for (v=0; v<b->getVertexCount(); ++v)
						nchunk.Vertices.push_back(((ox::video::S3DVertex*)b->getVertices())[v]);

					polyCount += b->getIndexCount();

					for (v=0; v<b->getIndexCount(); ++v)
						nchunk.Indices.push_back(b->getIndices()[v]);
				}

				StdOctTree = new OctTree<ox::video::S3DVertex>(StdMeshes, MinimalPolysPerNode);
				nodeCount = StdOctTree->nodeCount;
			}
			break;
		case ox::video::EVT_2TCOORDS:
			{
				for (int i=0; i<mesh->getMeshBufferCount(); ++i)
				{
					IMeshBuffer* b = mesh->getMeshBuffer(i);
					Materials.push_back(b->getMaterial());

					OctTree<ox::video::S3DVertex2TCoords>::SMeshChunk chunk;
					chunk.MaterialId = i;
					LightMapMeshes.push_back(chunk);
					OctTree<ox::video::S3DVertex2TCoords>::SMeshChunk& nchunk =
						LightMapMeshes[LightMapMeshes.size()-1];

					int v;

					for (v=0; v<b->getVertexCount(); ++v)
						nchunk.Vertices.push_back(((ox::video::S3DVertex2TCoords*)b->getVertices())[v]);

					polyCount += b->getIndexCount();

					for (v=0; v<b->getIndexCount(); ++v)
						nchunk.Indices.push_back(b->getIndices()[v]);
				}

				LightMapOctTree = new OctTree<ox::video::S3DVertex2TCoords>(LightMapMeshes, MinimalPolysPerNode);
				nodeCount = LightMapOctTree->nodeCount;
			}
			break;
		}
	}

	unsigned int endTime = os::Timer::getTime();
	char tmp[255];
	sprintf(tmp, "Needed %dms to create OctTree SceneNode.(%d nodes, %d polys)",
		endTime - beginTime, nodeCount, polyCount/3);
	os::Printer::log(tmp, ox::event::ELL_INFORMATION);

	return true;
}


//! returns the material based on the zero based index i. To get the amount
//! of materials used by this scene node, use getMaterialCount().
//! This function is needed for inserting the node into the scene hirachy on a
//! optimal position for minimizing renderstate changes, but can also be used
//! to directly modify the material of a scene node.
ox::video::SMaterial& COctTreeSceneNode::getMaterial(int i)
{
	if (i < 0 || i >= (int)Materials.size())
		return ISceneNode::getMaterial(i);

	return Materials[i];
}

//! returns amount of materials used by this scene node.
int COctTreeSceneNode::getMaterialCount()
{
	return Materials.size();
}


} // end namespace scene
} // end namespace daisy
