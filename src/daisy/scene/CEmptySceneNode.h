// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CEmptySceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_EMPTY_SCENE_NODE_H_INCLUDED__
#define __C_EMPTY_SCENE_NODE_H_INCLUDED__

#include "ox/scene/ISceneNode.h"
#include "ox/core/CAabbox3d.h"
#include "ox/scene/ISceneManager.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	class CEmptySceneNode : public ISceneNode
	{
	public:

		//! constructor
		CEmptySceneNode(ISceneNode* parent, ISceneManager* mgr, int id);

		//! returns the axis aligned bounding box of this node
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

		//! This method is called just before the rendering process of the whole scene.
		virtual void OnPreRender();

		//! does nothing.
		virtual void render();

	private:

		ox::core::CAabbox3d<float> Box;
	};

} // end namespace scene
} // end namespace daisy

#endif

