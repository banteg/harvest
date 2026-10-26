// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CTerrainSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/video/ColorPacking.h"
#include "ox/algo/CArrayFunctions.h"
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
	ox::video::IImage* ret = 0;

	ox::core::CDimension2d<int> hmapsize = heightmap->getDimension();
	ox::core::CDimension2d<int> idealsize(1,1);

	while(idealsize.Width < maxVtxSize.Width || idealsize.Width < hmapsize.Width)
		idealsize.Width <<= 1;

	while(idealsize.Height < maxVtxSize.Height || idealsize.Height < hmapsize.Height)
		idealsize.Height <<= 1;

	idealsize.Height+=1;
	idealsize.Width+=1;

	if (idealsize == hmapsize)
	{
		heightmap->grab();
		ret = heightmap;
	}
	else
	{
		char tmp[255];
		sprintf(tmp, "Resized heightmap to ideal size %dx%d (vtxblock size was: %dx%d)",
			idealsize.Width, idealsize.Height,
			maxVtxSize.Width, maxVtxSize.Height);
		os::Printer::log(tmp, ox::event::ELL_INFORMATION);

		video::CImage* h = new video::CImage((ox::video::ECOLOR_FORMAT)heightmap->getColorFormat(), heightmap);
		h->resizeTo(idealsize);
		ret = h;
	}

	return ret;
}


//! loads the terrain
bool CTerrainSceneNode::load(ox::video::IImage* texture, ox::video::IImage* inputheightmap,
	ox::video::ITexture* detailmap, const ox::core::CDimension2d<float>& stretchSize,
	const ox::core::CDimension2d<int>& vtxBlockSize, float maxHeight)
{
	unsigned int tm = os::Timer::getTime()/1000;

	if (!texture || !inputheightmap)
		return false;

	// get ideal size of vertex blocks
	ox::core::CDimension2d<int> maxVtxBlockSize =
		getBestBlockSize(vtxBlockSize);

	// resize heightmap to ideal size if necessary
	ox::video::IImage* heightmap = getIdealHeightMap(
		inputheightmap, maxVtxBlockSize);

	// store some variables

	char textureName[64];
	char tmp[255];

	ox::video::S3DVertex2TCoords vtx;
	vtx.Color = ox::video::SColor(255, 255, 255, 255);

	ox::core::CDimension2d<int> hMapSize= heightmap->getDimension();
	ox::core::CDimension2d<int> tMapSize= texture->getDimension();
	ox::core::CPosition2d<float> thRel((float)tMapSize.Width / (int)hMapSize.Width, (float)tMapSize.Height / (int)hMapSize.Height);
	ox::core::CPosition2d<int> processed(0,0);

	// create all vertex blocks

	while (processed.Y<hMapSize.Height)
	{
		while(processed.X<hMapSize.Width)
		{
			ox::core::CDimension2d<int> blockSize = maxVtxBlockSize;
			if (processed.X + blockSize.Width > hMapSize.Width)
				blockSize.Width = hMapSize.Width - processed.X;
			if (processed.Y + blockSize.Height > hMapSize.Height)
				blockSize.Height = hMapSize.Height - processed.Y;

			VertexBlocks.push_back(SVertexBlock());
			SVertexBlock& cvb = VertexBlocks[VertexBlocks.size()-1];
			cvb.Size = blockSize;

			int x,y;

			// add vertices of vertex block
			for (y=0; y<blockSize.Height; ++y)
				for (x=0; x<blockSize.Width; ++x)
				{
					ox::video::SColor clr = heightmap->getPixel(x+processed.X, y+processed.Y);
					float height = ((clr.getRed() + clr.getGreen() + clr.getBlue()) / 3.0f)/255.0f * maxHeight;

					vtx.Pos.set((float)(x+processed.X) * stretchSize.Width,
						height, (float)(y+processed.Y) * stretchSize.Height);

					vtx.TCoords.set((float)(x+0.5f) / ((float)blockSize.Width),
						(float)(y+0.5f) / ((float)blockSize.Height));

					cvb.Vertices.push_back(vtx);
				}

			// create all indices
			createIndexBuffers(cvb);

			// calculate bounding box
			calculateBoundingBox(cvb);

			// calculate all normals
			calculateNormals(cvb);

			// create texture for the block
			if (cvb.Vertices.size())
			{
				// create texture for this block
				ox::video::IImage* img = new video::CImage(texture,
					ox::core::CPosition2d<int>((int)(processed.X*thRel.X), (int)(processed.Y*thRel.Y)),
					ox::core::CDimension2d<int>((int)(blockSize.Width*thRel.X), (int)(blockSize.Height*thRel.Y)));

				sprintf(textureName, "terrain%d_%d", tm, VertexBlocks.size());

				cvb.Material.Lighting = false;
				cvb.Material.Texture1 =
					SceneManager->getVideoDriver()->addTexture(textureName, img);

				if (cvb.Material.Texture1)
				{
					sprintf(tmp, "Generated terrain texture (%dx%d): %s",
						cvb.Material.Texture1->getSize().Width,
						cvb.Material.Texture1->getSize().Height,
						textureName);
					os::Printer::log(tmp, ox::event::ELL_INFORMATION);
				}
				else
					os::Printer::log("Could not create terrain texture.", textureName, ox::event::ELL_ERROR);

				img->drop();
			}

            // keep on processing
			processed.X += maxVtxBlockSize.Width-1;
		}

		// keep on processing
		processed.X = 0;
		processed.Y += maxVtxBlockSize.Height-1;
	}

	calculateBoundingBox();
	heightmap->drop();

	return true;
}


//! creates index buffers for all level of details for a vertex block
void CTerrainSceneNode::createIndexBuffers(SVertexBlock& block)
{
	// search if we already have a fitting indexbuffer
	for (int i=0; i<(int)IndexArrayList.size(); ++i)
		if (IndexArrayList[i]->VertexBlockSize == block.Size)
		{
			block.IndexArray = IndexArrayList[i];
			return;
		}

	// add new index array
	IndexArrayList.push_back(new SIndexArray());
	SIndexArray& idxarray = *IndexArrayList[IndexArrayList.size()-1];
    idxarray.VertexBlockSize = block.Size;
	block.IndexArray = IndexArrayList[IndexArrayList.size()-1];

	int x,y;

	// create levels of detail without borders

	ox::TArray<int> widths;
	ox::TArray<int> heights;

	for (int lodfactor=1;
		(block.Size.Height/lodfactor)>2 &&
		(block.Size.Width/lodfactor)>2; lodfactor*=2)
	{
		idxarray.LoDs.push_back(SLoD());
		int lodnr = idxarray.LoDs.size()-1;
		idxarray.LoDs[lodnr].Buffer.push_back(SIndexBuffer());
		SIndexBuffer& idxbfr2 = idxarray.LoDs[lodnr].Buffer[0];

		idxbfr2.OrigVertexBlockSize = block.Size;
		idxbfr2.IndexedVertexBlockSize.Width = (block.Size.Width - lodfactor - 1) / lodfactor;
		idxbfr2.IndexedVertexBlockSize.Height = (block.Size.Height - lodfactor - 1) / lodfactor;

		if (ox::algo::binarySearchPos(widths.begin(), widths.end(), idxbfr2.IndexedVertexBlockSize.Width)==-1)
			widths.push_back(idxbfr2.IndexedVertexBlockSize.Width);

		if (ox::algo::binarySearchPos(heights.begin(), heights.end(), idxbfr2.IndexedVertexBlockSize.Height)==-1)
			heights.push_back(idxbfr2.IndexedVertexBlockSize.Height);

		// generate indices
		for (y=0; y<(int)((block.Size.Height-lodfactor-1)/lodfactor); ++y)
			for (x=0; x<(int)((block.Size.Width-lodfactor-1)/lodfactor); ++x)
		{
			int c = ((y*lodfactor)*block.Size.Width) + (x*lodfactor);

			idxbfr2.Indices.push_back(c);
			idxbfr2.Indices.push_back(c + (block.Size.Width*lodfactor));
			idxbfr2.Indices.push_back(c + lodfactor);

			idxbfr2.Indices.push_back(c + lodfactor);
			idxbfr2.Indices.push_back(c + (block.Size.Width*lodfactor));
			idxbfr2.Indices.push_back(c + lodfactor + (block.Size.Width*lodfactor));
		}
	}

	// now every LOD has got its default indices. The indices to fit
	// the different borders are still missing. We create them now.

	for (int l=0; l<(int)idxarray.LoDs.size(); ++l)
	{
		// copy already created indices
		SLoD& lod = idxarray.LoDs[l];
		SIndexBuffer virginBuffer = lod.Buffer[0];
		lod.Buffer.clear();

		for (int w=0; w<(int)widths.size(); ++w)
			for (int h=0; h<(int)heights.size(); ++h)
			{
				lod.Buffer.push_back(virginBuffer);
				SIndexBuffer& buf = lod.Buffer[lod.Buffer.size()-1];

				int wVertexStep = buf.OrigVertexBlockSize.Width / buf.IndexedVertexBlockSize.Width;
				int hVertexStep = buf.OrigVertexBlockSize.Height / buf.IndexedVertexBlockSize.Height;

				// add %width% indices on top
				//if (widths[w] <= buf.IndexedVertexBlockSize.Width)
				{
					// this block has more details

					//buf.Indices.push_back(buf.IndexedVertexBlockSize.Height*hVertexStep);
					//buf.Indices.push_back((buf.IndexedVertexBlockSize.Height*hVertexStep)+(buf.OrigVertexBlockSize.Width*wVertexStep));
					//buf.Indices.push_back(buf.OrigVertexBlockSize.Width+(buf.OrigVertexBlockSize.Width*wVertexStep)-1);

					// add first basic triangle
					// |------
					// |    /
					// |  /
					// |/
					//
					buf.Indices.push_back(buf.IndexedVertexBlockSize.Height*wVertexStep);
					buf.Indices.push_back((buf.IndexedVertexBlockSize.Height*hVertexStep)+(buf.OrigVertexBlockSize.Height*hVertexStep));
					buf.Indices.push_back(buf.OrigVertexBlockSize.Height+(buf.OrigVertexBlockSize.Height*wVertexStep)-1);

				}
				/*else
				{
					// this block has smaller or similar details

					int vpb = widths[w] / buf.IndexedVertexBlockSize.Width;
					buf.Indices.clear();
				}*/

			}
	}
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
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, SNRT_DEFAULT);

	ISceneNode::OnPreRender();
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
