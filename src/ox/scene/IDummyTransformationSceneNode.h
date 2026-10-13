// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IDummyTransformationSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef __I_DUMMY_TRANSFORMATION_SCENE_NODE_H_INCLUDED__
#define __I_DUMMY_TRANSFORMATION_SCENE_NODE_H_INCLUDED__

#include "ISceneNode.h"
#include "ox/core/CMatrix4.h"
#include "ox/scene/IAnimatedMeshSceneNode.h"
#include "ox/scene/ISceneManager.h"

namespace ox
{
namespace scene
{

//! Dummy scene node for adding additional transformations to the scene graph.
/** This scene node does not render itself, and does not respond to set/getPosition,
set/getRotation and set/getScale. Its just a simple scene node that takes a
matrix as relative transformation, making it possible to insert any transformation
anywhere into the scene graph.
This scene node is for example used by the IAnimatedMeshSceneNode for emulating
joint scene nodes when playing skeletal animations.
*/
class IDummyTransformationSceneNode : public ISceneNode
{
public:

	//! constructor
	IDummyTransformationSceneNode(ISceneNode* parent, ISceneManager* mgr, int id)
		: ISceneNode(parent, mgr, id) {}

    //! Returns a reference to the current relative transformation matrix.
	//! This is the matrix, this scene node uses instead of scale, translation
	//! and rotation.
	virtual ox::core::CMatrix4& getRelativeTransformationMatrix() = 0;
};

} // end namespace scene
} // end namespace ox


#endif
