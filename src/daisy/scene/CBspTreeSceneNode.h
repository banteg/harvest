// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CBspTreeSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_BSP_TREE_SCENE_NODE_H_INCLUDED__
#define __C_BSP_TREE_SCENE_NODE_H_INCLUDED__

#include "ox/scene/IBspTreeSceneNode.h"
#include "ox/core/CAabbox3d.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;
	class CBspTree;

	//! implementation of the IBspTreeSceneNode
	class CBspTreeSceneNode : public IBspTreeSceneNode
	{
	public:

		//! constructor
		CBspTreeSceneNode(ISceneNode* parent, ISceneManager* mgr, int id);

		//! destructor
		virtual ~CBspTreeSceneNode();

		virtual void OnPreRender();

		//! renders the node.
		virtual void render();

		//! returns the axis aligned bounding box of this node
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

		//! creates the tree
		bool createTree(IMesh* mesh);

	private:

		CBspTree* Tree;
		ox::core::CAabbox3d<float> Box;
	};

} // end namespace scene
} // end namespace daisy

#endif
