// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CDummyTransformationSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_DUMMY_TRANSFORMATION_SCENE_NODE_H_INCLUDED__
#define __C_DUMMY_TRANSFORMATION_SCENE_NODE_H_INCLUDED__

#include "ox/scene/IDummyTransformationSceneNode.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	class CDummyTransformationSceneNode : public IDummyTransformationSceneNode
	{
	public:

		//! constructor
		CDummyTransformationSceneNode(ISceneNode* parent, ISceneManager* mgr, int id);

		//! destructor
		~CDummyTransformationSceneNode();

		//! returns the axis aligned bounding box of this node
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

		//! Returns a reference to the current relative transformation matrix.
		//! This is the matrix, this scene node uses instead of scale, translation
		//! and rotation.
		virtual ox::core::CMatrix4& getRelativeTransformationMatrix();

		//! Returns the relative transformation of the scene node.
		virtual ox::core::CMatrix4 getRelativeTransformation() const;

		//! does nothing.
		virtual void render() {};

	private:

		ox::core::CMatrix4 RelativeTransformationMatrix;
		ox::core::CAabbox3d<float> Box;
	};

} // end namespace scene
} // end namespace daisy

#endif

