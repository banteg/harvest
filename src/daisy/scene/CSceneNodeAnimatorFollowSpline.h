// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneNodeAnimatorFollowSpline.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_SCENE_NODE_ANIMATOR_FOLLOW_SPLINE_H_INCLUDED__
#define __C_SCENE_NODE_ANIMATOR_FOLLOW_SPLINE_H_INCLUDED__

#include "ox/scene/ISceneNode.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/ISceneNodeAnimator.h"
#include "ox/TArray.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;
	//! Scene node animator based free code Matthias Gall wrote and sent in. (Most of 
	//! this code is written by him, I only modified bits.)
	class CSceneNodeAnimatorFollowSpline : public ISceneNodeAnimator
	{
	public:
	
		//! constructor
		CSceneNodeAnimatorFollowSpline(unsigned int startTime, 
			const ox::TArray< ox::core::CVector3d<float> >& points,
			float speed = 1.0f, float tightness = 0.5f);

		//! destructor
		virtual ~CSceneNodeAnimatorFollowSpline();

		//! animates a scene node
		virtual void animateNode(ISceneNode* node, unsigned int timeMs);

	protected:

		//! clamps a the value idx to fit into range 0..size-1
		int clamp(int idx, int size);

		ox::TArray< ox::core::CVector3d<float> > Points;
		float Speed;
		float Tightness;
		unsigned int StartTime;
		unsigned int NumPoints;
	};


} // end namespace scene
} // end namespace daisy

#endif

