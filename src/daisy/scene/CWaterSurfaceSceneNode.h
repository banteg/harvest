// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CWaterSurfaceSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_WATER_SURFACE_SCENE_NODE_H_INCLUDED__
#define __C_WATER_SURFACE_SCENE_NODE_H_INCLUDED__

#include "CMeshSceneNode.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	class CWaterSurfaceSceneNode : public CMeshSceneNode
	{
	public:

		//! constructor
		CWaterSurfaceSceneNode(float waveHeight, float waveSpeed, float waveLenght,
			IMesh* mesh, ISceneNode* parent, ISceneManager* mgr,	int id,
			const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0),
			const ox::core::CVector3d<float>& rotation = ox::core::CVector3d<float>(0,0,0),
			const ox::core::CVector3d<float>& scale = ox::core::CVector3d<float>(1.0f, 1.0f, 1.0f));

		//! destructor
		virtual ~CWaterSurfaceSceneNode();

		//! frame
		virtual void OnPreRender();

	private:

		void animateWaterSurface();
		float WaveLength;
		float WaveSpeed;
		float WaveHeight;
		IMesh* OriginalMesh;
	};

} // end namespace scene
} // end namespace daisy

#endif
