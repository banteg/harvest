// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CWaterSurfaceSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CVector3d.h"
#include "ox/video/S3DVertex2TCoords.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/ISceneNode.h"
#include "CWaterSurfaceSceneNode.h"
#include "ox/video/IVideoDriver.h"
#include "ox/scene/ISceneManager.h"
#include "ox/video/S3DVertex.h"
#include "ox/scene/SViewFrustrum.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/scene/IMeshManipulator.h"
#include "ox/scene/SMesh.h"
#include "daisy/os.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;



//! constructor
CWaterSurfaceSceneNode::CWaterSurfaceSceneNode(float waveHeight, float waveSpeed, float waveLength,
		IMesh* mesh, ISceneNode* parent, ISceneManager* mgr, int id,
		const ox::core::CVector3d<float>& position, const ox::core::CVector3d<float>& rotation,
		const ox::core::CVector3d<float>& scale)
: CMeshSceneNode(mesh, parent, mgr, id, position, rotation, scale), OriginalMesh(0)
{
	WaveSpeed = waveSpeed;//300.0f;
	WaveLength = waveLength;//10.0f;
	WaveHeight = waveHeight;//2.0f;

	// create copy of the mesh
	if (!mesh)
		return;

	IMesh* clone = SceneManager->getMeshManipulator()->createMeshCopy(mesh);
	OriginalMesh = Mesh;
	Mesh = clone;
}



//! destructor
CWaterSurfaceSceneNode::~CWaterSurfaceSceneNode()
{
	if (OriginalMesh)
		OriginalMesh->drop();
}



//! frame
void CWaterSurfaceSceneNode::OnPreRender()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, SNRT_DEFAULT);

	animateWaterSurface();

	ISceneNode::OnPreRender();
}


void CWaterSurfaceSceneNode::animateWaterSurface()
{
	if (!Mesh)
		return;

	int meshBufferCount = Mesh->getMeshBufferCount();
	float time = os::Timer::getTime() / WaveSpeed;

    for (int b=0; b<meshBufferCount; ++b)
	{
		int vtxCnt = Mesh->getMeshBuffer(b)->getVertexCount();

		switch((int)Mesh->getMeshBuffer(b)->getVertexType())
		{
		case ox::video::EVT_STANDARD:
			{
				ox::video::S3DVertex* v =
					(ox::video::S3DVertex*)Mesh->getMeshBuffer(b)->getVertices();

				ox::video::S3DVertex* v2 =
					(ox::video::S3DVertex*)OriginalMesh->getMeshBuffer(b)->getVertices();

				for (int i=0; i<vtxCnt; ++i)
				{
					v[i].Pos.Y = v2[i].Pos.Y +
					((float)sin(((v2[i].Pos.X/WaveLength) + time)) * WaveHeight) +
					((float)cos(((v2[i].Pos.Z/WaveLength) + time)) * WaveHeight);
				}

			}
			break;
		case ox::video::EVT_2TCOORDS:
			{
				ox::video::S3DVertex2TCoords* v =
					(ox::video::S3DVertex2TCoords*)Mesh->getMeshBuffer(b)->getVertices();

				ox::video::S3DVertex2TCoords* v2 =
					(ox::video::S3DVertex2TCoords*)OriginalMesh->getMeshBuffer(b)->getVertices();

				for (int i=0; i<vtxCnt; ++i)
				{
					v[i].Pos.Y = v2[i].Pos.Y +
					((float)sin(((v2[i].Pos.X/WaveLength) + time)) * WaveHeight) +
					((float)cos(((v2[i].Pos.Z/WaveLength) + time)) * WaveHeight);
				}
			}
			break;
		}// end switch

	}// end for all mesh buffers

	SceneManager->getMeshManipulator()->recalculateNormals(Mesh);

}


} // end namespace scene
} // end namespace daisy
