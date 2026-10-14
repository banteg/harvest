// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IBspTreeSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef __I_BSP_TREE_SCENE_NODE_H_INCLUDED__
#define __I_BSP_TREE_SCENE_NODE_H_INCLUDED__

#include "ISceneNode.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/ISceneManager.h"
#include "IMesh.h"

namespace ox
{
namespace scene
{

	//! Scene Node which mananages the geometry with a binary space partition tree.
	/** The render speed can be maximized with this method. Binary space partition
	trees are sometimes useful when rendering complex indoor scenes. It usually takes
	some time to build up a bsp tree, so it would be good to do this offline, and store
	the result on disk.
	Note that the implementation of the BspTree in the Irrlicht Engine is currently
	not complete a little bit buggy. You may want to use the OctTree instead, it already works.
	Use ISceneManager::addOctTreeSceneNode() for that.
	*/
	class IBspTreeSceneNode : public ISceneNode
	{
	public:

		//! Constructor
		IBspTreeSceneNode(ISceneNode* parent, ISceneManager* mgr, int id)
			: ISceneNode(parent, mgr, id, ox::core::CVector3d<float>(0,0,0),
				ox::core::CVector3d<float>(0,0,0), ox::core::CVector3d<float>(1.0f, 1.0f, 1.0f)) {}

		//! Destructor
		virtual ~IBspTreeSceneNode() {};
	};

} // end namespace scene
} // end namespace ox

#endif

