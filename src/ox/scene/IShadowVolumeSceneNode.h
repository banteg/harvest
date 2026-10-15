// Copyright (C) 2002-2003 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IShadowVolumeSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef __I_SHADOW_VOLUME_SCENE_NODE_H_INCLUDED__
#define __I_SHADOW_VOLUME_SCENE_NODE_H_INCLUDED__

#include "ISceneNode.h"
#include "ox/scene/ISceneManager.h"
#include "IMesh.h"

namespace ox
{
namespace scene
{

	//! Scene node for rendering a shadow volume into a stencil buffer.
	class IShadowVolumeSceneNode : public ISceneNode
	{
	public:

		//! constructor
		IShadowVolumeSceneNode(ISceneNode* parent, ISceneManager* mgr, int id)
			: ISceneNode(parent, mgr, id) {}

		//! sets the mesh from which the shadow volume should be generated.
		virtual void setMeshToRenderFrom(IMesh* mesh) = 0;
	};

} // end namespace scene
} // end namespace ox

#endif
