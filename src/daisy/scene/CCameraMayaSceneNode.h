// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CCameraMayaSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_CAMERA_MAYA_SCENE_NODE_H_INCLUDED__
#define __C_CAMERA_MAYA_SCENE_NODE_H_INCLUDED__

#include "CCameraSceneNode.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"
#include "ox/event/IEventReceiver.h"
#include "ox/core/CVector2d.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	class CCameraMayaSceneNode : public CCameraSceneNode
	{
	public:

		//! constructor
		CCameraMayaSceneNode(ISceneNode* parent, ISceneManager* mgr, int id,
			float rotateSpeed = -1500.0f, float zoomSpeed = 200.0f, float translationSpeed = 100.0f);

		//! destructor
		virtual ~CCameraMayaSceneNode();

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

		//! sets the position of the node; the maya camera reads it as its zoom
		virtual void setPosition(const ox::core::CVector3d<float>& newpos);

	private:

		void allKeysUp();
		void animate();
		bool isMouseKeyDown(int key);

		bool MouseKeys[3];

		ox::core::CVector3d<float> Pos;
		bool zooming, rotating, moving, translating;
		float zoomSpeed;
		float translateSpeed;
		float rotateSpeed;
		float rotateStartX, rotateStartY;
		float zoomStartX, zoomStartY;
		float translateStartX, translateStartY;
		float currentZoom;
		float rotX, rotY;
		float oldTargetX, oldTargetY, oldTargetZ;

		ox::core::CVector2d<float> MousePos;
	};

} // end namespace
} // end namespace

#endif
