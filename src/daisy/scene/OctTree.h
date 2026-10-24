// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/OctTree.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_OCT_TREE_H_INCLUDED__
#define __C_OCT_TREE_H_INCLUDED__

#include "ox/scene/SViewFrustrum.h"
#include "ox/core/CVector3d.h"
#include "ox/core/CPlane3d.h"
#include "ox/video/S3DVertex.h"
#include "ox/core/CAabbox3d.h"
#include "ox/TArray.h"
#include <memory.h>

namespace daisy
{

//! template octtree. T must be a vertex type which has a member
//! called .Pos, which is a core::vertex3df position.
template <class T>
class OctTree
{
public:

	int nodeCount;

	struct SMeshChunk
	{
		ox::TArray<T> Vertices;
		ox::TArray<unsigned short> Indices;
		int MaterialId;
	};

	struct SIndexChunk
	{
		ox::TArray<unsigned short> Indices;
		int MaterialId;
	};

	struct SIndexData
	{
		unsigned short* Indices;
		int CurrentSize;
		int MaxSize;
	};



	//! constructor
	OctTree(const ox::TArray<SMeshChunk>& meshes, int minimalPolysPerNode=128)
	{
		nodeCount = 0;

		IndexDataCount = meshes.size();
		IndexData = new SIndexData[IndexDataCount];

		// construct array of all indices

		ox::TArray<SIndexChunk>* indexChunks = new ox::TArray<SIndexChunk>;
		SIndexChunk ic;

		for (unsigned int i=0; i<meshes.size(); ++i)
		{
			IndexData[i].CurrentSize = 0;
			IndexData[i].MaxSize = meshes[i].Indices.size();
			IndexData[i].Indices = new unsigned short[IndexData[i].MaxSize];

			ic.MaterialId = meshes[i].MaterialId;
			(*indexChunks).push_back(ic);

			SIndexChunk& tic = (*indexChunks)[i];

			for (unsigned int t=0; t<meshes[i].Indices.size(); ++t)
				tic.Indices.push_back(meshes[i].Indices[t]);
		}

		// create tree

		Root = new OctTreeNode(nodeCount, 0, meshes, indexChunks, minimalPolysPerNode);
	}

	//! returns all ids of polygons partially or full enclosed
	//! by this bounding box.
	void calculatePolys(const ox::core::CAabbox3d<float>& box)
	{
		for (int i=0; i<IndexDataCount; ++i)
			IndexData[i].CurrentSize = 0;

		Root->getPolys(box, IndexData);
	}

	//! returns all ids of polygons partially or full enclosed
	//! by a view frustrum.
	void calculatePolys(const ox::scene::SViewFrustrum& frustrum)
	{
		for (int i=0; i<IndexDataCount; ++i)
			IndexData[i].CurrentSize = 0;

		Root->getPolys(frustrum, IndexData);
	}


	SIndexData* getIndexData()
	{
		return IndexData;
	}

	int getIndexDataCount()
	{
		return IndexDataCount;
	}

	// for debug purposes only, renders the bounding boxes of the tree
	void renderBoundingBoxes(const ox::core::CAabbox3d<float>& box,
		ox::TArray< ox::core::CAabbox3d<float> >&outBoxes)
	{
		Root->renderBoundingBoxes(box, outBoxes);
	}

	//! destructor
	~OctTree()
	{
		for (int i=0; i<IndexDataCount; ++i)
			delete [] IndexData[i].Indices;

		delete [] IndexData;
		delete Root;
	}

private:


	// private inner class
	class OctTreeNode
	{
	public:

		// constructor
		OctTreeNode(int& nodeCount, int currentdepth,
			const ox::TArray<SMeshChunk>& allmeshdata,
			ox::TArray<SIndexChunk>* indices,
			int minimalPolysPerNode) : IndexData(0)
		{
			depth = currentdepth+1;
			++nodeCount;

			unsigned int i; // new ISO for scoping problem with different compilers

			for (i=0; i<8; ++i)
				Children[i] = 0;

			if ((*indices).empty())
			{
				delete indices;
				return;
			}

			bool found = false;

			// find first point for bounding box

			for (i=0; i<(*indices).size(); ++i)
				if (!(*indices)[i].Indices.empty())
				{
					Box.reset(allmeshdata[i].Vertices[(*indices)[i].Indices[0]].Pos);
					found = true;
					break;
				}

			if (!found)
			{
				delete indices;
				return;
			}

			int totalPrimitives = 0;

			// now letz calculate our bounding box
			for (i=0; i<(*indices).size(); ++i)
			{
				totalPrimitives += (*indices)[i].Indices.size();
				for (unsigned int j=0; j<(*indices)[i].Indices.size(); ++j)
					Box.addInternalPoint(allmeshdata[i].Vertices[(*indices)[i].Indices[j]].Pos);
			}

			ox::core::CVector3d<float> middle = Box.getCenter();
			ox::core::CVector3d<float> edges[8];
			Box.getEdges(edges);

			// calculate all children
			ox::core::CAabbox3d<float> box;

			if (totalPrimitives > minimalPolysPerNode && !Box.isEmpty())
			for (int ch=0; ch<8; ++ch)
			{
				box.reset(middle);
				box.addInternalPoint(edges[ch]);

				// create indices for child
				ox::TArray<SIndexChunk>* cindexChunks = new ox::TArray<SIndexChunk>;

				bool added = false;

				for (unsigned int i=0; i<allmeshdata.size(); ++i)
				{
					SIndexChunk ic;
					ic.MaterialId = allmeshdata[i].MaterialId;
					(*cindexChunks).push_back(ic);

					SIndexChunk& tic = (*cindexChunks)[i];

					for (unsigned int t=0; t<(*indices)[i].Indices.size(); t+=3)
					{
						if (box.isPointInside(allmeshdata[i].Vertices[(*indices)[i].Indices[t]].Pos) &&
							box.isPointInside(allmeshdata[i].Vertices[(*indices)[i].Indices[t+1]].Pos) &&
							box.isPointInside(allmeshdata[i].Vertices[(*indices)[i].Indices[t+2]].Pos))
						{
							tic.Indices.push_back((*indices)[i].Indices[t]);
							tic.Indices.push_back((*indices)[i].Indices[t+1]);
							tic.Indices.push_back((*indices)[i].Indices[t+2]);

							(*indices)[i].Indices.erase((*indices)[i].Indices.begin() + (int)t, (*indices)[i].Indices.begin() + (int)(t + 3));

							t-=3;

							added = true;
						}
					}
				}

				if (added)
					Children[ch] = new OctTreeNode(nodeCount, depth,
						allmeshdata, cindexChunks, minimalPolysPerNode);
				else
					delete cindexChunks;

			} // end for all possible children

			IndexData = indices;
		}



		// destructor
		~OctTreeNode()
		{
			delete IndexData;

			for (int i=0; i<8; ++i)
				delete Children[i];
		}



		// returns all ids of polygons partially or full enclosed
		// by this bounding box.
		void getPolys(const ox::core::CAabbox3d<float>& box, SIndexData* idxdata)
		{
			if (Box.intersectsWithBox(box))
			{
				int cnt = (*IndexData).size();
				int i; // new ISO for scoping problem in some compilers

				for (i=0; i<cnt; ++i)
				{
					int idxcnt = (*IndexData)[i].Indices.size();

					if (idxcnt)
					{
						memcpy(&idxdata[i].Indices[idxdata[i].CurrentSize],
							&(*IndexData)[i].Indices[0], idxcnt * sizeof(short));
						idxdata[i].CurrentSize += idxcnt;
					}
				}

				for (i=0; i<8; ++i)
					if (Children[i])
						Children[i]->getPolys(box, idxdata);
			}
		}



		// returns all ids of polygons partially or full enclosed
		// by the view frustrum.
		void getPolys(const ox::scene::SViewFrustrum& frustrum, SIndexData* idxdata)
		{
			int i; // new ISO for scoping problem in some compilers

			bool visible = true;

			ox::core::CVector3d<float> edges[8];
			Box.getEdges(edges);

			for (i=0; i<ox::scene::SViewFrustrum::VF_PLANE_COUNT; ++i)
			{
				bool boxInFrustrum = false;

				for (int j=0; j<8; ++j)
				if (frustrum.planes[i].classifyPointRelation(edges[j]) != ox::core::ISREL3D_BACK)
				{
					boxInFrustrum = true;
					break;
				}

				if (!boxInFrustrum)
				{
					visible = false;
					break;
				}
			}

			if (visible)
			{
				int cnt = (*IndexData).size();

				for (i=0; i<cnt; ++i)
				{
					int idxcnt = (*IndexData)[i].Indices.size();

					if (idxcnt)
					{
						memcpy(&idxdata[i].Indices[idxdata[i].CurrentSize],
							&(*IndexData)[i].Indices[0], idxcnt * sizeof(short));
						idxdata[i].CurrentSize += idxcnt;
					}
				}
			}

			for (i=0; i<8; ++i)
				if (Children[i])
					Children[i]->getPolys(frustrum, idxdata);
		}



		void renderBoundingBoxes(const ox::core::CAabbox3d<float>& box,
			ox::TArray< ox::core::CAabbox3d<float> >&outBoxes)
		{
			if (Box.intersectsWithBox(box))
			{
				outBoxes.push_back(Box);

				for (int i=0; i<8; ++i)
					if (Children[i])
						Children[i]->renderBoundingBoxes(box, outBoxes);
			}
		}

	private:

		ox::core::CAabbox3d<float> Box;
		ox::TArray<SIndexChunk>* IndexData;
		OctTreeNode* Children[8];
		int depth;
	};


	OctTreeNode* Root;
	SIndexData* IndexData;
	int IndexDataCount;
};

} // end namespace

#endif
