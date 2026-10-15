// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CMeshSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_MESH_SCENE_NODE_H_INCLUDED__
#define __C_MESH_SCENE_NODE_H_INCLUDED__

#include "ox/scene/ISceneNode.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/ISceneManager.h"
#include "ox/TArray.h"
#include "ox/scene/IMesh.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	class CMeshSceneNode : public ISceneNode
	{
	public:

		//! constructor
		CMeshSceneNode(IMesh* mesh, ISceneNode* parent, ISceneManager* mgr,	int id,
			const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0),
			const ox::core::CVector3d<float>& rotation = ox::core::CVector3d<float>(0,0,0),
			const ox::core::CVector3d<float>& scale = ox::core::CVector3d<float>(1.0f, 1.0f, 1.0f));

		//! destructor
		virtual ~CMeshSceneNode();

		//! frame
		virtual void OnPreRender();

		//! renders the node.
		virtual void render();

		//! returns the axis aligned bounding box of this node
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

		//! returns the material based on the zero based index i. To get the amount
		//! of materials used by this scene node, use getMaterialCount().
		//! This function is needed for inserting the node into the scene hirachy on a
		//! optimal position for minimizing renderstate changes, but can also be used
		//! to directly modify the material of a scene node.
		virtual ox::video::SMaterial& getMaterial(int i);

		//! returns amount of materials used by this scene node.
		virtual int getMaterialCount();

	protected:

		ox::TArray<ox::video::SMaterial> Materials;
		ox::core::CAabbox3d<float> Box;
		IMesh* Mesh;
	};

} // end namespace scene
} // end namespace daisy

#endif
