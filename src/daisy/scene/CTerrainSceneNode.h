// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CTerrainSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_TERRAIN_SCENE_NODE_H_INCLUDED__
#define __C_TERRAIN_SCENE_NODE_H_INCLUDED__

#include "ox/scene/ITerrainSceneNode.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CVector2d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/IImage.h"
#include "ox/video/ITexture.h"
#include "ox/video/S3DVertex2TCoords.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"
#include "ox/TArray.h"
#include "ox/video/S3DVertex.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! ox::core::A terrain scene node using simple geomipmapping.
class CTerrainSceneNode : public ITerrainSceneNode
{
public:

	//! constructor
	CTerrainSceneNode(ISceneNode* parent, ISceneManager* mgr, int id,
		const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0),
		const ox::core::CVector3d<float>& rotation = ox::core::CVector3d<float>(0,0,0),
		const ox::core::CVector3d<float>& scale = ox::core::CVector3d<float>(1.0f, 1.0f, 1.0f));

	//! destructor
	~CTerrainSceneNode();

	//! loads the terrain
	bool load(ox::video::IImage* texture, ox::video::IImage* heightmap,
		ox::video::ITexture* detailmap,
		const ox::core::CDimension2d<float>& stretchSize,
		const ox::core::CDimension2d<int>& defaultVertexBlockSize,
		float maxHeight);

	//! renders the node.
	virtual void render();

	virtual void OnPreRender();

	virtual ox::video::SMaterial& getMaterial(int i);

	virtual int getMaterialCount();

	//! Returns the axis aligned, not transformed bounding box of this node.
	virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

	//! Calculates height of a point in the terrain
	virtual bool getHeight(ox::core::CVector2d<float> pos, float& outHeight);

	//! Sets the Level of detail to a fixed level for all tiles.
	//! Only use this for debug purposes.
	//! \param i: Level to add to the current debug level of detail,
	//! may be negative. To disable debugging LOD, set this to 0.
	virtual void setDebugLOD(int i);

private:

	struct SIndexBuffer
	{
		int WidthOfUpperBlock;
		int HeightOfLeftBlock;
		ox::TArray<unsigned short> Indices;
		ox::core::CDimension2d<int> OrigVertexBlockSize;
		ox::core::CDimension2d<int> IndexedVertexBlockSize; // size of this lod

		bool operator ==(const SIndexBuffer& other) const
		{
			return WidthOfUpperBlock == other.WidthOfUpperBlock &&
				HeightOfLeftBlock == other.HeightOfLeftBlock;
		}

		bool operator <(const SIndexBuffer& other) const
		{
			return WidthOfUpperBlock < other.WidthOfUpperBlock ||
				(WidthOfUpperBlock == other.WidthOfUpperBlock &&
				HeightOfLeftBlock < other.HeightOfLeftBlock);
		}
	};

	//! Structure for holding index buffers for the same level of detail
	//! and the same vertex block size, but differen border indices
	struct SLoD
	{
		ox::TArray<SIndexBuffer> Buffer;
	};

	//! Index for a specific vertex block size
	struct SIndexArray
	{
		ox::core::CDimension2d<int> VertexBlockSize;
		ox::TArray<SLoD> LoDs; // index buffers for every level of detail
	};

	// Vertex block
    struct SVertexBlock
	{
		SVertexBlock()
			: IndexArray(0)
		{
		}

		ox::core::CDimension2d<int> Size; // in pixels
		SIndexArray* IndexArray; // array with indizes
		ox::video::SMaterial Material;
		ox::core::CAabbox3d<float> Box;

		ox::TArray<ox::video::S3DVertex2TCoords> Vertices;
	};

	const ox::core::CDimension2d<int> getBestBlockSize(ox::core::CDimension2d<int> input);
	ox::video::IImage* getIdealHeightMap(ox::video::IImage* heightmap, const ox::core::CDimension2d<int> maxVtxSize);


	//! creates index buffers for all level of details for a vertex block
	void createIndexBuffers(SVertexBlock& block);

	//! calculates the bounding box of the vertex block
	void calculateBoundingBox(SVertexBlock& block);

	//! calculates all normals of the vertex block
	void calculateNormals(SVertexBlock& block);

	//! calculates the bounding box for all vertex blocks
	void calculateBoundingBox();

	//! returns corresponding index buffer for vertex block
	SIndexBuffer* getIndexBuffer(SVertexBlock& block);

	ox::TArray<SVertexBlock> VertexBlocks;
	ox::TArray<SIndexArray*> IndexArrayList;

	ox::core::CAabbox3d<float> Box;
	int debugLOD;
	bool debuggingLOD;
};

} // end namespace scene
} // end namespace daisy


#endif
