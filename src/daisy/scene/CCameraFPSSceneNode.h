// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CCameraFPSSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_CAMERA_FPS_SCENE_NODE_H_INCLUDED__
#define __C_CAMERA_FPS_SCENE_NODE_H_INCLUDED__

#include "ox/gui/ICursorControl.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"
#include "ox/event/IEventReceiver.h"
#include "ox/Keycodes.h"
#include "CCameraSceneNode.h"
#include "ox/core/CVector2d.h"
#include "SKeyMap.h"
#include "ox/TArray.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	class CCameraFPSSceneNode : public CCameraSceneNode
	{
	public:

		//! constructor
		CCameraFPSSceneNode(ISceneNode* parent, ISceneManager* mgr, 
			ox::gui::ICursorControl* cursorControl, int id,
			float rotateSpeed, float moveSpeed,
			SKeyMap* keyMapArray, int keyMapSize);

		//! destructor
		virtual ~CCameraFPSSceneNode();

		//! It is possible to send mouse and key events to the camera. Most cameras
		//! may ignore this input, but camera scene nodes which are created for 
		//! example with ox::scene::ISceneManager::addMayaCameraSceneNode or
		//! ox::scene::ISceneManager::addMeshViewerCameraSceneNode, may want to get this input
		//! for changing their position, look at target or whatever. 
		virtual bool OnEvent(const ox::event::SEvent& event);

		//! onPostRender is called just after rendering the whole scene.
		//! nodes may calculate or store animations here, and may do other useful things,
		//! dependent on what they are.
		virtual void OnPostRender(unsigned int timeMs);

		//! sets the look at target of the camera
		//! \param pos: Look at target of the camera.
		virtual void setTarget(const ox::core::CVector3d<float>& pos);

	private:

		struct SCamKeyMap
		{
			SCamKeyMap() {};
			SCamKeyMap(int a, ox::EKEY_CODE k) : action(a), keycode(k) {}

			int action;
			ox::EKEY_CODE keycode;
		};

		void allKeysUp();
		void animate();

		bool CursorKeys[4];

		float MoveSpeed;
		float RotateSpeed;

		//float RotationX;
		//float RotationY;

		ox::gui::ICursorControl* CursorControl;

		bool firstUpdate;
		int LastAnimationTime;

		ox::core::CVector3d<float> TargetVector;
		ox::TArray<SCamKeyMap> KeyMap;
	};

} // end namespace
} // end namespace

#endif

