// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CCameraMayaSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CDimension2d.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/SViewFrustrum.h"
#include "ox/event/IEventReceiver.h"
#include "CCameraMayaSceneNode.h"
#include "ox/video/IVideoDriver.h"
#include "ox/scene/ISceneManager.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CCameraMayaSceneNode::CCameraMayaSceneNode(ISceneNode* parent, ISceneManager* mgr, int id,
	 float rs, float zs, float ts)
: CCameraSceneNode(parent, mgr, id)
{
	zooming = false;
	rotating = false;
	moving = false;
	translating = false;
	zoomSpeed = zs;
	rotateSpeed = rs;
	translateSpeed = ts;
	currentZoom = 70.0f;
	//Pos.set(0.0f, 70.0f, 0.0f);
	Target.set(0.0f, 0.0f, 0.0f);
	rotX = 0;
	rotY = 0;
	oldTargetX = Target.X;
	oldTargetY = Target.Y;
	oldTargetZ = Target.Z;

	allKeysUp();
	recalculateViewArea();
}


//! destructor
CCameraMayaSceneNode::~CCameraMayaSceneNode()
{
}


//! It is possible to send mouse and key events to the camera. Most cameras
//! may ignore this input, but camera scene nodes which are created for 
//! example with ox::scene::ISceneManager::addMayaCameraSceneNode or
//! ox::scene::ISceneManager::addMeshViewerCameraSceneNode, may want to get this input
//! for changing their position, look at target or whatever.
bool CCameraMayaSceneNode::OnEvent(const ox::event::SEvent& event)
{
	if (event.EventType != ox::event::EET_MOUSE_INPUT_EVENT ||
		!InputReceiverEnabled)
		return false;

	switch(event.MouseInput.Event)
	{
	case ox::event::EMIE_LMOUSE_PRESSED_DOWN:
		MouseKeys[0] = true;
		break;
	case ox::event::EMIE_RMOUSE_PRESSED_DOWN:
		MouseKeys[2] = true;
		break;
	case ox::event::EMIE_MMOUSE_PRESSED_DOWN:
		MouseKeys[1] = true;
		break;
	case ox::event::EMIE_LMOUSE_LEFT_UP:
		MouseKeys[0] = false;
		break;
	case ox::event::EMIE_RMOUSE_LEFT_UP:
		MouseKeys[2] = false;
		break;
	case ox::event::EMIE_MMOUSE_LEFT_UP:
		MouseKeys[1] = false;
		break;
	case ox::event::EMIE_MOUSE_MOVED:
		{
			ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();
			if (driver)
			{
				ox::core::CDimension2d<int> ssize = SceneManager->getVideoDriver()->getScreenSize();
				MousePos.X = event.MouseInput.X / (float)ssize.Width;
				MousePos.Y = event.MouseInput.Y / (float)ssize.Height;
			}
		}
		break;
	}
	return true;
}

//! onPostRender is called just after rendering the whole scene.
//! nodes may calculate or store animations here, and may do other useful things,
//! dependent on what they are.
void CCameraMayaSceneNode::OnPostRender(unsigned int timeMs)
{
	animate();

	setPosition(Pos);
	updateAbsolutePosition();

	// This scene node cannot be animated by scene node animators.
}


bool CCameraMayaSceneNode::isMouseKeyDown(int key)
{
	return MouseKeys[key];
}



void CCameraMayaSceneNode::animate()
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


void CCameraMayaSceneNode::allKeysUp()
{
	for (int i=0; i<3; ++i)
		MouseKeys[i] = false;
}


} // end namespace
} // end namespace

