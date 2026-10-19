// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CTerrainSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CPlane3d.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CVector2d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/IImage.h"
#include "ox/video/ITexture.h"
#include "ox/video/S3DVertex2TCoords.h"
#include "ox/video/SColor.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/ITerrainSceneNode.h"
#include "ox/event/ILogger.h"
#include "ox/TArray.h"
#include "CTerrainSceneNode.h"
#include "ox/video/IVideoDriver.h"
#include "ox/scene/ISceneManager.h"
#include "daisy/os.h"
#include "daisy/video/Null/CImage.h"
#include <stdio.h>

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CTerrainSceneNode::CTerrainSceneNode(ISceneNode* parent,
									 ISceneManager* mgr, int id,
									 const ox::core::CVector3d<float>& position,
									 const ox::core::CVector3d<float>& rotation,
									 const ox::core::CVector3d<float>& scale)
: ITerrainSceneNode(parent, mgr, id, position, rotation, scale),
	debuggingLOD(false), debugLOD(0)
{
}


//! destructor
CTerrainSceneNode::~CTerrainSceneNode()
{
	for (int i=0; i<(int)IndexArrayList.size(); ++i)
		delete IndexArrayList[i];
}


const ox::core::CDimension2d<int> CTerrainSceneNode::getBestBlockSize(
	ox::core::CDimension2d<int> input)
{
	if (input.Width < 16) input.Width = 16;
	if (input.Width > 128) input.Width = 128;
	if (input.Height < 16) input.Height = 16;
	if (input.Height > 128) input.Height = 128;

	ox::core::CDimension2d<int> output;

	output.Width = 0x01;
	while(output.Width < input.Width)
		output.Width <<= 1;

	output.Height = 0x01;
	while(output.Height < input.Height)
		output.Height <<= 1;

	++output.Height;
	++output.Width;

	return output;
}


ox::video::IImage* CTerrainSceneNode::getIdealHeightMap(ox::video::IImage* heightmap,
													const ox::core::CDimension2d<int> maxVtxSize)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


//! loads the terrain
bool CTerrainSceneNode::load(ox::video::IImage* texture, ox::video::IImage* inputheightmap,
	ox::video::ITexture* detailmap, const ox::core::CDimension2d<float>& stretchSize,
	const ox::core::CDimension2d<int>& vtxBlockSize, float maxHeight)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


//! creates index buffers for all level of details for a vertex block
void CTerrainSceneNode::createIndexBuffers(SVertexBlock& block)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


//! calculates all normals of the vertex block
void CTerrainSceneNode::calculateNormals(SVertexBlock& block)
{
	if (!block.IndexArray || block.IndexArray->LoDs.empty())
		return;

	int indexCnt = block.IndexArray->LoDs[0].Buffer[0].Indices.size();
	unsigned short* idx = &block.IndexArray->LoDs[0].Buffer[0].Indices[0];

	for (int i=0; i<indexCnt; i+=3)
	{
		ox::core::CPlane3d<float> p(
			block.Vertices[idx[i+0]].Pos,
			block.Vertices[idx[i+1]].Pos,
			block.Vertices[idx[i+2]].Pos);
		p.Normal.normalize();

		block.Vertices[idx[i+0]].Normal = p.Normal;
		block.Vertices[idx[i+1]].Normal = p.Normal;
		block.Vertices[idx[i+2]].Normal = p.Normal;
	}
}


//! calculates the bounding box of the vertex block
void CTerrainSceneNode::calculateBoundingBox(SVertexBlock& block)
{
	if (block.Vertices.empty())
		return;

	block.Box.reset(block.Vertices[0].Pos);

	for (int i=1; i<(int)block.Vertices.size(); ++i)
		block.Box.addInternalPoint(block.Vertices[i].Pos);
}



//! Calculates height of a point in the terrain
bool CTerrainSceneNode::getHeight(ox::core::CVector2d<float> pos, float& outHeight)
{
	return false;
}



//! renders the node.
void CTerrainSceneNode::render()
{
	ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();
	driver->setTransform(ox::video::ETS_WORLD, AbsoluteTransformation);

	// simply draw all
	for (int i=0; i<(int)VertexBlocks.size(); ++i)
	{
		SVertexBlock& block = VertexBlocks[i];
		SIndexBuffer* idx = getIndexBuffer(block);

		if (idx)
		{
			driver->setMaterial(block.Material);

			const unsigned short* indices = &idx->Indices[0];
			const ox::video::S3DVertex2TCoords* vertices = &block.Vertices[0];
			int vertexCnt = block.Vertices.size();
			int triangleCount = idx->Indices.size() / 3;

			driver->drawIndexedTriangleList(
				vertices, vertexCnt, indices, triangleCount);
		}
	}
}



//! Returns the axis aligned, not transformed bounding box of this node.
const ox::core::CAabbox3d<float>& CTerrainSceneNode::getBoundingBox() const
{
	return Box;
}


//! calculates the bounding box for all vertex blocks
void CTerrainSceneNode::calculateBoundingBox()
{
	if (VertexBlocks.empty())
		return;

	Box = VertexBlocks[0].Box;

	for (int i=1; i<(int)VertexBlocks.size(); ++i)
		Box.addInternalBox(VertexBlocks[i].Box);
}


//! returns corresponding index buffer for vertex block
CTerrainSceneNode::SIndexBuffer* CTerrainSceneNode::getIndexBuffer(SVertexBlock& block)
{
	int cnt = block.IndexArray->LoDs.size();
	if (!cnt)
		return 0;

	int t= debugLOD;
	if (t > cnt-1)
		t = cnt-1;
	return &block.IndexArray->LoDs[t].Buffer[0];
}


void CTerrainSceneNode::OnPreRender()
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


ox::video::SMaterial& CTerrainSceneNode::getMaterial(int i)
{
	if (i<0 || i>=(int)VertexBlocks.size())
        return *((ox::video::SMaterial*)0);

	return VertexBlocks[i].Material;
}


int CTerrainSceneNode::getMaterialCount()
{
	return VertexBlocks.size();
}


void CTerrainSceneNode::setDebugLOD(int i)
{
	debuggingLOD = (i != 0);

	debugLOD+=i;
	if (debugLOD <0)
		debugLOD = 0;
	if (debugLOD >= (int)IndexArrayList[0]->LoDs.size())
		debugLOD = IndexArrayList[0]->LoDs.size()-1;
}




} // end namespace scene
} // end namespace daisy
