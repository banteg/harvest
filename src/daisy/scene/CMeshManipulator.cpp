// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CMeshManipulator.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CPlane3d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/S3DVertex2TCoords.h"
#include "ox/scene/IMeshBuffer.h"
#include "CMeshManipulator.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/SMesh.h"
#include "ox/scene/SMeshBuffer.h"
#include "ox/scene/SMeshBufferLightMap.h"
#include "ox/scene/IAnimatedMesh.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! Constructor
CMeshManipulator::CMeshManipulator()
{
}


//! destructor
CMeshManipulator::~CMeshManipulator()
{
}

//! Flips the direction of surfaces. Changes backfacing triangles to frontfacing
//! triangles and vice versa.
//! \param mesh: Mesh on which the operation is performed.
void CMeshManipulator::flipSurfaces(ox::scene::IMesh* mesh) const
{
	if (!mesh)
		return;

	int bcount = mesh->getMeshBufferCount();
	for (int b=0; b<bcount; ++b)
	{
		IMeshBuffer* buffer = mesh->getMeshBuffer(b);
		int idxcnt = buffer->getIndexCount();
		unsigned short* idx = buffer->getIndices();
		int tmp;

		for (int i=0; i<idxcnt; i+=3)
		{
			tmp = idx[i+1];
			idx[i+1] = idx[i+2];
			idx[i+2] = tmp;
		}
	}
}



//! Sets the alpha vertex color value of the whole mesh to a new value
//! \param mesh: Mesh on which the operation is performed.
void CMeshManipulator::setVertexColorAlpha(ox::scene::IMesh* mesh, int alpha) const
{
	if (!mesh)
		return;

	int i = 0;

	int bcount = mesh->getMeshBufferCount();
	for (int b=0; b<bcount; ++b)
	{
		IMeshBuffer* buffer = mesh->getMeshBuffer(b);
		void* v = buffer->getVertices();
		int vtxcnt = buffer->getVertexCount();

		switch((int)buffer->getVertexType())
		{
		case ox::video::EVT_STANDARD:
			for (; i<vtxcnt; ++i)
				((ox::video::S3DVertex*)v)[i].Color.setAlpha(alpha);
			break;
		case ox::video::EVT_2TCOORDS:
			for (; i<vtxcnt; ++i)
				((ox::video::S3DVertex2TCoords*)v)[i].Color.setAlpha(alpha);
			break;
		}
	}
}



//! Recalculates all normals of the mesh.
//! \param mesh: Mesh on which the operation is performed.
void CMeshManipulator::recalculateNormals(ox::scene::IMesh* mesh) const
{
	if (!mesh)
		return;

	int bcount = mesh->getMeshBufferCount();
	for (int b=0; b<bcount; ++b)
	{
		IMeshBuffer* buffer = mesh->getMeshBuffer(b);
		int vtxcnt = buffer->getVertexCount();
		int idxcnt = buffer->getIndexCount();
		unsigned short* idx = buffer->getIndices();

		switch((int)buffer->getVertexType())
		{
		case ox::video::EVT_STANDARD:
			{
				ox::video::S3DVertex* v =
					(ox::video::S3DVertex*)buffer->getVertices();

				for (int i=0; i<idxcnt; i+=3)
				{
					ox::core::CPlane3d<float> p(v[idx[i+0]].Pos, v[idx[i+1]].Pos, v[idx[i+2]].Pos);
					p.Normal.normalize();
					v[idx[i+0]].Normal = p.Normal;
					v[idx[i+1]].Normal = p.Normal;
					v[idx[i+2]].Normal = p.Normal;
				}
			}
			break;
		case ox::video::EVT_2TCOORDS:
			{
				ox::video::S3DVertex2TCoords* v =
					(ox::video::S3DVertex2TCoords*)buffer->getVertices();

				for (int i=0; i<idxcnt; i+=3)
				{
					ox::core::CPlane3d<float> p(v[idx[i+0]].Pos, v[idx[i+1]].Pos, v[idx[i+2]].Pos);
					p.Normal.normalize();
					v[idx[i+0]].Normal = p.Normal;
					v[idx[i+1]].Normal = p.Normal;
					v[idx[i+2]].Normal = p.Normal;
				}
			}
			break;
		}
	}
}


//! Scales the whole mesh.
//! \param mesh: Mesh on which the operation is performed.
void CMeshManipulator::scaleMesh(ox::scene::IMesh* mesh, const ox::core::CVector3d<float>& scale) const
{
	if (!mesh)
		return;

	ox::core::CAabbox3d<float> meshbox;

	int bcount = mesh->getMeshBufferCount();
	for (int b=0; b<bcount; ++b)
	{
		int i = 0;

		IMeshBuffer* buffer = mesh->getMeshBuffer(b);
		void* v = buffer->getVertices();
		int vtxcnt = buffer->getVertexCount();
		ox::core::CAabbox3d<float> bufferbox;

		switch((int)buffer->getVertexType())
		{
		case ox::video::EVT_STANDARD:
			if (vtxcnt != 0)
				bufferbox.reset(((ox::video::S3DVertex*)v)[0].Pos * scale);
			for (; i<vtxcnt; ++i)
			{
				((ox::video::S3DVertex*)v)[i].Pos *= scale;
				bufferbox.addInternalPoint(((ox::video::S3DVertex*)v)[i].Pos);
			}
			break;
		case ox::video::EVT_2TCOORDS:
			if (vtxcnt != 0)
				bufferbox.reset(((ox::video::S3DVertex2TCoords*)v)[0].Pos * scale);
			for (; i<vtxcnt; ++i)
			{
				((ox::video::S3DVertex2TCoords*)v)[i].Pos *= scale;
				bufferbox.addInternalPoint(((ox::video::S3DVertex2TCoords*)v)[i].Pos);
			}
			break;
		}

		buffer->getBoundingBox() = bufferbox;

		if (b == 0)
			meshbox.reset(buffer->getBoundingBox());
		else
			meshbox.addInternalBox(buffer->getBoundingBox());
	}

	mesh->getBoundingBox() = meshbox;
}


//! Recalculates the bounding box for a meshbuffer
void CMeshManipulator::recalculateBoundingBox(ox::scene::IMeshBuffer* buffer) const
{
	void* v = buffer->getVertices();
	int vtxcnt = buffer->getVertexCount();
	ox::core::CAabbox3d<float> box;

	int i = 1;

	switch(buffer->getVertexType())
	{
	case ox::video::EVT_STANDARD:
		if (vtxcnt != 0)
			box.reset(((ox::video::S3DVertex*)v)[0].Pos);
		for (; i<vtxcnt; ++i)
			box.addInternalPoint(((ox::video::S3DVertex*)v)[i].Pos);
		break;
	case ox::video::EVT_2TCOORDS:
		if (vtxcnt != 0)
			box.reset(((ox::video::S3DVertex2TCoords*)v)[0].Pos);
		for (; i<vtxcnt; ++i)
			box.addInternalPoint(((ox::video::S3DVertex2TCoords*)v)[i].Pos);
		break;
	}

	buffer->getBoundingBox() = box;
}



//! Clones a static IMesh into a modifyable SMesh.
SMesh* CMeshManipulator::createMeshCopy(ox::scene::IMesh* mesh) const
{
	if (!mesh)
		return 0;

	SMesh* clone = new SMesh();

	int meshBufferCount = mesh->getMeshBufferCount();

    for (int b=0; b<meshBufferCount; ++b)
	{
		int vtxCnt = mesh->getMeshBuffer(b)->getVertexCount();
		int idxCnt = mesh->getMeshBuffer(b)->getIndexCount();
		const unsigned short* idx = mesh->getMeshBuffer(b)->getIndices();

		switch((int)mesh->getMeshBuffer(b)->getVertexType())
		{
		case ox::video::EVT_STANDARD:
			{
				SMeshBuffer* buffer = new SMeshBuffer();
				buffer->Material = mesh->getMeshBuffer(b)->getMaterial();

				ox::video::S3DVertex* v =
					(ox::video::S3DVertex*)mesh->getMeshBuffer(b)->getVertices();

				int i;

				for (i=0; i<vtxCnt; ++i)
					buffer->Vertices.push_back(v[i]);

				for (i=0; i<idxCnt; ++i)
					buffer->Indices.push_back(idx[i]);

				clone->addMeshBuffer(buffer);
				buffer->drop();
			}
			break;
		case ox::video::EVT_2TCOORDS:
			{
				SMeshBufferLightMap* buffer = new SMeshBufferLightMap();
				buffer->Material = mesh->getMeshBuffer(b)->getMaterial();

				ox::video::S3DVertex2TCoords* v =
					(ox::video::S3DVertex2TCoords*)mesh->getMeshBuffer(b)->getVertices();

				int i;

				for (i=0; i<vtxCnt; ++i)
					buffer->Vertices.push_back(v[i]);

				for (i=0; i<idxCnt; ++i)
					buffer->Indices.push_back(idx[i]);

				clone->addMeshBuffer(buffer);
				buffer->drop();
			}
			break;
		}// end switch

	}// end for all mesh buffers

	clone->BoundingBox = mesh->getBoundingBox();
	return clone;
}



//! Creates a planar texture mapping on the mesh
//! \param mesh: Mesh on which the operation is performed.
//! \param resolution: resolution of the planar mapping. This is the value
//! specifying which is the releation between world space and
//! texture coordinate space.
void CMeshManipulator::makePlanarTextureMapping(ox::scene::IMesh* mesh, float resolution=0.01f) const
{
	if (!mesh)
		return;

	int bcount = mesh->getMeshBufferCount();
	for (int b=0; b<bcount; ++b)
	{
		IMeshBuffer* buffer = mesh->getMeshBuffer(b);
		int vtxcnt = buffer->getVertexCount();
		int idxcnt = buffer->getIndexCount();
		unsigned short* idx = buffer->getIndices();

		switch((int)buffer->getVertexType())
		{
		case ox::video::EVT_STANDARD:
			{
				ox::video::S3DVertex* v =
					(ox::video::S3DVertex*)buffer->getVertices();

				for (int i=0; i<idxcnt; i+=3)
				{
					ox::core::CPlane3d<float> p(v[idx[i+0]].Pos, v[idx[i+1]].Pos, v[idx[i+2]].Pos);
					p.Normal.normalize();

					p.Normal.X = (float)(fabs(p.Normal.X));
					p.Normal.Y = (float)(fabs(p.Normal.Y));
					p.Normal.Z = (float)(fabs(p.Normal.Z));

					// calculate planar mapping worldspace coordinates

					if (p.Normal.X > p.Normal.Y && p.Normal.X > p.Normal.Z)
					{
						for (int o=0; o<3; ++o)
						{
							v[idx[i+o]].TCoords.X = v[idx[i+o]].Pos.Y * resolution;
							v[idx[i+o]].TCoords.Y = v[idx[i+o]].Pos.Z * resolution;
						}
					}
					else
					if (p.Normal.Y > p.Normal.X && p.Normal.Y > p.Normal.Z)
					{
						for (int o=0; o<3; ++o)
						{
							v[idx[i+o]].TCoords.X = v[idx[i+o]].Pos.X * resolution;
							v[idx[i+o]].TCoords.Y = v[idx[i+o]].Pos.Z * resolution;
						}
					}
					else
					{
						for (int o=0; o<3; ++o)
						{
							v[idx[i+o]].TCoords.X = v[idx[i+o]].Pos.X * resolution;
							v[idx[i+o]].TCoords.Y = v[idx[i+o]].Pos.Y * resolution;
						}
					}
				}
			}
			break;
		case ox::video::EVT_2TCOORDS:
			{
				ox::video::S3DVertex2TCoords* v =
					(ox::video::S3DVertex2TCoords*)buffer->getVertices();

				for (int i=0; i<idxcnt; i+=3)
				{
					ox::core::CPlane3d<float> p(v[idx[i+0]].Pos, v[idx[i+1]].Pos, v[idx[i+2]].Pos);
					p.Normal.normalize();

					p.Normal.X = (float)(fabs(p.Normal.X));
					p.Normal.Y = (float)(fabs(p.Normal.Y));
					p.Normal.Z = (float)(fabs(p.Normal.Z));

					// calculate planar mapping worldspace coordinates

					if (p.Normal.X > p.Normal.Y && p.Normal.X > p.Normal.Z)
					{
						for (int o=0; o<3; ++o)
						{
							v[idx[i+o]].TCoords.X = v[idx[i+o]].Pos.Y * resolution;
							v[idx[i+o]].TCoords.Y = v[idx[i+o]].Pos.Z * resolution;
						}
					}
					else
					if (p.Normal.Y > p.Normal.X && p.Normal.Y > p.Normal.Z)
					{
						for (int o=0; o<3; ++o)
						{
							v[idx[i+o]].TCoords.X = v[idx[i+o]].Pos.X * resolution;
							v[idx[i+o]].TCoords.Y = v[idx[i+o]].Pos.Z * resolution;
						}
					}
					else
					{
						for (int o=0; o<3; ++o)
						{
							v[idx[i+o]].TCoords.X = v[idx[i+o]].Pos.X * resolution;
							v[idx[i+o]].TCoords.Y = v[idx[i+o]].Pos.Y * resolution;
						}
					}
				}

			}
			break;
		}
	}
}


//! Returns amount of polygons in mesh.
int CMeshManipulator::getPolyCount(ox::scene::IMesh* mesh) const
{
	if (!mesh)
		return 0;

	int trianglecount = 0;

	for (int g=0; g<mesh->getMeshBufferCount(); ++g)
		trianglecount += mesh->getMeshBuffer(g)->getIndexCount() / 3;

	return trianglecount;
}


//! Returns amount of polygons in mesh.
int CMeshManipulator::getPolyCount(ox::scene::IAnimatedMesh* mesh) const
{
	if (mesh && mesh->getFrameCount() != 0)
		return getPolyCount(mesh->getMesh(0));

	return 0;
}

} // end namespace scene
} // end namespace daisy
