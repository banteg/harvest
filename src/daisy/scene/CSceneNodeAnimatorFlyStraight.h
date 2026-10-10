// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneNodeAnimatorFlyStraight.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_SCENE_NODE_ANIMATOR_FLY_STRAIGHT_H_INCLUDED__
#define __C_SCENE_NODE_ANIMATOR_FLY_STRAIGHT_H_INCLUDED__

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
	class CSceneNodeAnimatorFlyStraight : public ISceneNodeAnimator
	{
	public:

		//! constructor
		CSceneNodeAnimatorFlyStraight(const ox::core::CVector3d<float>& startPoint, 
									  const ox::core::CVector3d<float>& endPoint, unsigned int timeForWay,
									  bool loop, unsigned int now);

		//! destructor
		virtual ~CSceneNodeAnimatorFlyStraight();

		//! animates a scene node
		virtual void animateNode(ISceneNode* node, unsigned int timeMs);

	private:

		ox::core::CVector3d<float> Start;
		ox::core::CVector3d<float> End;
		ox::core::CVector3d<float> Vector;
		float WayLength;
		float TimeFactor;
		unsigned int StartTime;
		unsigned int EndTime;
		unsigned int TimeForWay;
		bool Loop;
	};


} // end namespace scene
} // end namespace daisy

#endif

