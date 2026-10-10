// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneNodeAnimatorFlyCircle.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_SCENE_NODE_ANIMATOR_FLY_CIRCLE_H_INCLUDED__
#define __C_SCENE_NODE_ANIMATOR_FLY_CIRCLE_H_INCLUDED__

#include "ox/scene/ISceneNode.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/ISceneNodeAnimator.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;
	class CSceneNodeAnimatorFlyCircle : public ISceneNodeAnimator
	{
	public:

		//! constructor
		CSceneNodeAnimatorFlyCircle(unsigned int time, const ox::core::CVector3d<float>& center, float radius, float speed);

		//! destructor
		virtual ~CSceneNodeAnimatorFlyCircle();

		//! animates a scene node
		virtual void animateNode(ISceneNode* node, unsigned int timeMs);

	private:

		ox::core::CVector3d<float> Center;
		float Radius;
		float Speed;
		unsigned int StartTime;
	};


} // end namespace scene
} // end namespace daisy

#endif

