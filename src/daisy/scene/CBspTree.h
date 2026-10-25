// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CBspTree.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_BSPTREE_H_INCLUDED__
#define __C_BSPTREE_H_INCLUDED__

#include "ox/TArray.h"
#include "ox/core/CVector3d.h"
#include "ox/video/S3DVertex.h"
#include "ox/scene/SViewFrustrum.h"
#include "ox/core/CPlane3d.h"
#include "ox/video/SMaterial.h"
#include "ox/core/CAabbox3d.h"
#include "ox/scene/IMeshBuffer.h"
#include "ox/scene/ICameraSceneNode.h"

// The Statistics are used to locate errors and to
// find a good dimension for the renderbuffer.

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	struct SBspTreeStats
	{
		unsigned int leafCounts;
		unsigned int polySplits;
		unsigned int nodeCount;

		unsigned int originalTotalVertexCount;
		unsigned int originalTotalIndexCount;
		unsigned int totalIndexCount;
		unsigned int totalVertexCount;

		unsigned int *indexCount; // counts for every material type
		unsigned int *vertexCount;
		ox::video::SMaterial *materialMap; // map for all materials

		unsigned int renderCullings;
		unsigned int materialCount;
		unsigned int renderedIndices;

		ox::TArray<ox::core::CAabbox3d<float> > boxes; // bounding boxes of all leafs.
	};

	const float BSP_ROUND_ERROR = 0.00001f;

	class CBspTree
	{
		struct BspPoly
		{
			BspPoly() {};

			BspPoly& operator = (const BspPoly& other)
			{
				vertices = other.vertices;
				indices = other.indices;
				wasSplitter = other.wasSplitter;
				plane = other.plane;
				material = other.material;

				return * this;
			}

			ox::TArray<ox::video::S3DVertex> vertices;
			ox::TArray<unsigned short> indices;

			bool wasSplitter;           // was the poly already a splitter?
			ox::core::CPlane3d<float> plane; // plane of this poly

			unsigned int material;      // number of material index

			bool operator==(const BspPoly& other) const
			{
				if (vertices.size()  != other.vertices.size() ||
					indices.size() != other.indices.size() ||
					plane != other.plane)
				return false;

				// compare indices

				unsigned int i; // new ISO for scoping not liked by some compilers

				for (i=0; i<indices.size(); ++i)
					if (indices[i] != other.indices[i])
						return false;

				// compare vertices

				for (i=0; i<vertices.size(); ++i)
					if (!(vertices[i].Pos == other.vertices[i].Pos && vertices[i].Normal == other.vertices[i].Normal && vertices[i].Color.color == other.vertices[i].Color.color && vertices[i].TCoords == other.vertices[i].TCoords))
						return false;

				return true;
			}
		};

		struct BspNode
		{
			BspNode()
				: isLeaf(false), front(0), back(0)
			{}

			ox::core::CPlane3d<float> plane; // Teilungsebene
			ox::core::CAabbox3d<float> bbox; // Bounding box

			bool isLeaf;                // Node oder Leaf

			ox::TArray<BspPoly> polys; // pointer to bsp polys
			BspNode* front;             // frontliste
			BspNode* back;              // backliste
		};

		enum BoxCullingResult
		{
			BCR_COMPLETE_INSIDE,
			BCR_COMPLETE_OUTSIDE,
			BCP_CLIPPED
		};

	public:

		struct RenderBuffer
		{
			ox::TArray< ox::video::S3DVertex > vertices;
			ox::TArray< unsigned short > indices;
		};

		//! creates multi material bsptree, each buffer has another material
		CBspTree(const ox::scene::IMeshBuffer* array, unsigned int bufferCount);

		//! destructor
		virtual ~CBspTree();

		const RenderBuffer* getRenderBuffer(unsigned int materialNr) const;

		unsigned int getMaterialCount();

		//! aabbRendering enables fast but inaccurate aabb-box rendering
		void render(const SViewFrustrum* camArea, const ox::core::CVector3d<float>& camPos, bool aabbRendering = false);

		//! returns stats of the bsptree
		const SBspTreeStats* getStats();


	private:

		//! creates root of tree
		void createRoot(const ox::scene::IMeshBuffer* array, unsigned int bufferCount);

		void createBspTree(BspNode& node);

		// liefert die anzahl der gesamten vertices im node zurück
		int createBoundingBox(const BspPoly* poly, unsigned int polyCount, ox::core::CAabbox3d<float>& outBox);

		bool findSplitter(BspPoly* poly, unsigned int polyCount, ox::core::CPlane3d<float>& outSplitter);
		void sortPolys(const BspNode& parent);

		void render(BspNode& node, const SViewFrustrum* camArea, const ox::core::CVector3d<float>& camPos);
		void renderBoxCulling(BspNode& node, const SViewFrustrum* camArea, const ox::core::CVector3d<float>& camPos);
		void renderLeaf(BspNode& node);

		ox::core::EIntersectionRelation3D classifyPoly(const ox::core::CPlane3d<float>& plane, const BspPoly& poly);
		ox::core::CPlane3d<float> planeFromPoly(const BspPoly& poly);
		void splitPoly(const BspPoly& poly, const ox::core::CPlane3d<float>& plane, BspPoly& frontSplit, BspPoly& backSplit);

		void deleteTree(BspNode *pNode);
		void createStats(const ox::scene::IMeshBuffer* array, unsigned int MaterialCount);
		void deleteStats();

		void createRenderBuffer();
		void countPolyVertizes(const BspNode& node);
		void countPolyIndizes(const BspNode& node);
		BoxCullingResult cullBoxAgainsView(const ox::core::CAabbox3d<float>& box, const SViewFrustrum* camArea);

		bool polyExistsInNode(BspPoly* poly, BspNode* node, unsigned int polysToCheck);

		#ifdef _DEBUG
		void debugOutputLeaf(const BspNode& node);
		void debugValidateLeaf(const BspNode& node);
		#endif

		inline int abs(int a)
		{
		  return a > 0 ? a : -a;
        }

		BspNode root;
		SBspTreeStats stats;
		RenderBuffer* renderBuffer;
	};


} // end namespace
} // end namespace


#endif // __BSPTREE_H_INCLUDED__
