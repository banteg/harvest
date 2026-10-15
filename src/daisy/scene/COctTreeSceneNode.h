// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/COctTreeSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_OCT_TREE_SCENE_NODE_H_INCLUDED__
#define __C_OCT_TREE_SCENE_NODE_H_INCLUDED__

#include "ox/scene/ISceneNode.h"
#include "ox/core/CAabbox3d.h"
#include "ox/video/S3DVertex.h"
#include "ox/video/S3DVertex2TCoords.h"
#include "ox/video/SMaterial.h"
#include "ox/scene/ISceneManager.h"
#include "ox/TArray.h"
#include "ox/scene/IMesh.h"
#include "OctTree.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;
	//! implementation of the IBspTreeSceneNode
	class COctTreeSceneNode : public ISceneNode
	{
	public:

		//! constructor
		COctTreeSceneNode(ISceneNode* parent, ISceneManager* mgr, int id,
			int minimalPolysPerNode=128);

		//! destructor
		virtual ~COctTreeSceneNode();

		virtual void OnPreRender();

		//! renders the node.
		virtual void render();

		//! returns the axis aligned bounding box of this node
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

		//! creates the tree
		bool createTree(IMesh* mesh);

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

		OctTree<ox::video::S3DVertex>* StdOctTree;
		ox::TArray< OctTree<ox::video::S3DVertex>::SMeshChunk > StdMeshes;

		OctTree<ox::video::S3DVertex2TCoords>* LightMapOctTree;
		ox::TArray< OctTree<ox::video::S3DVertex2TCoords>::SMeshChunk > LightMapMeshes;

		ox::video::E_VERTEX_TYPE vertexType;
		ox::TArray< ox::video::SMaterial > Materials;

		int MinimalPolysPerNode;
	};

} // end namespace scene
} // end namespace daisy

#endif
