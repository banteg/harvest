// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneNodeAnimatorDelete.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_SCENE_NODE_ANIMATOR_DELETE_H_INCLUDED__
#define __C_SCENE_NODE_ANIMATOR_DELETE_H_INCLUDED__

#include "ox/scene/ISceneNode.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/ISceneNodeAnimator.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;
	class CSceneNodeAnimatorDelete : public ISceneNodeAnimator
	{
	public:

		//! constructor
		CSceneNodeAnimatorDelete(ISceneManager* manager, unsigned int when);

		//! destructor
		virtual ~CSceneNodeAnimatorDelete();

		//! animates a scene node
		virtual void animateNode(ISceneNode* node, unsigned int timeMs);

	private:

		unsigned int DeleteTime;
		ISceneManager* SceneManager;
	};


} // end namespace scene
} // end namespace daisy

#endif

