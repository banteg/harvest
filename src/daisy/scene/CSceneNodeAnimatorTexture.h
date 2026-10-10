// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneNodeAnimatorTexture.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_SCENE_NODE_ANIMATOR_TEXTURE_H_INCLUDED__
#define __C_SCENE_NODE_ANIMATOR_TEXTURE_H_INCLUDED__

#include "ox/TArray.h"
#include "ox/video/ITexture.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/ISceneNodeAnimator.h"
#include "ox/scene/ISceneNode.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;
	class CSceneNodeAnimatorTexture : public ISceneNodeAnimator
	{
	public:

		//! constructor
		CSceneNodeAnimatorTexture(const ox::TArray<ox::video::ITexture*>& textures,
			int timePerFrame, bool loop, unsigned int now);

		//! destructor
		virtual ~CSceneNodeAnimatorTexture();

		//! animates a scene node
		virtual void animateNode(ISceneNode* node, unsigned int timeMs);

	private:

		ox::TArray<ox::video::ITexture*> Textures;
		unsigned int TimePerFrame;
		unsigned int StartTime;
		unsigned int EndTime;
		bool Loop;
	};


} // end namespace scene
} // end namespace daisy

#endif

