// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CDummyTransformationSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/scene/IDummyTransformationSceneNode.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"
#include "CDummyTransformationSceneNode.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CDummyTransformationSceneNode::CDummyTransformationSceneNode(
	ISceneNode* parent, ISceneManager* mgr, int id)
	: IDummyTransformationSceneNode(parent, mgr, id)
{
	setAutomaticCulling(false);
}



//! destructor
CDummyTransformationSceneNode::~CDummyTransformationSceneNode()
{
}



//! returns the axis aligned bounding box of this node
const ox::core::CAabbox3d<float>& CDummyTransformationSceneNode::getBoundingBox() const
{
	return Box;
}



//! Returns a reference to the current relative transformation matrix.
//! This is the matrix, this scene node uses instead of scale, translation
//! and rotation.
ox::core::CMatrix4& CDummyTransformationSceneNode::getRelativeTransformationMatrix()
{
	return RelativeTransformationMatrix;
}


//! Returns the relative transformation of the scene node.
ox::core::CMatrix4 CDummyTransformationSceneNode::getRelativeTransformation() const
{
	return RelativeTransformationMatrix;
}


} // end namespace scene
} // end namespace daisy
