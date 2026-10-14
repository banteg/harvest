// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSkyBoxSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_SKY_BOX_SCENE_NODE_H_INCLUDED__
#define __C_SKY_BOX_SCENE_NODE_H_INCLUDED__

#include "ox/scene/ISceneNode.h"
#include "ox/core/CAabbox3d.h"
#include "ox/video/ITexture.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/ISceneManager.h"
#include "ox/video/S3DVertex.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	// Skybox, rendererd with zbuffer turned off, before all other nodes.
	class CSkyBoxSceneNode : public ISceneNode
	{
	public:

		//! constructor
		CSkyBoxSceneNode(ox::video::ITexture* top, ox::video::ITexture* bottom, ox::video::ITexture* left,
			ox::video::ITexture* right, ox::video::ITexture* front, ox::video::ITexture* back,
			ISceneNode* parent, ISceneManager* mgr, int id);

		//! destructor
		virtual ~CSkyBoxSceneNode();

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

	private:

		ox::core::CAabbox3d<float> Box;
		unsigned short Indices[6];
		ox::video::S3DVertex Vertices[4*6];
		ox::video::SMaterial Material[6];
	};

} // end namespace scene
} // end namespace daisy

#endif

