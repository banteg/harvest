// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CCameraFPSSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CMatrix4.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CVector2d.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/ISceneNodeAnimator.h"
#include "ox/gui/ICursorControl.h"
#include "ox/event/IEventReceiver.h"
#include "ox/TList.h"
#include "CCameraFPSSceneNode.h"
#include "ox/video/IVideoDriver.h"
#include "ox/scene/ISceneManager.h"
#include "daisy/os.h"
#include "ox/Keycodes.h"
#include "stdio.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CCameraFPSSceneNode::CCameraFPSSceneNode(ISceneNode* parent, ISceneManager* mgr,
		ox::gui::ICursorControl* cursorControl, int id, float rotateSpeed , float moveSpeed,
		SKeyMap* keyMapArray, int keyMapSize)
: CCameraSceneNode(parent, mgr, id), CursorControl(cursorControl),
	MoveSpeed(moveSpeed), RotateSpeed(rotateSpeed), /*RotationX(0.0f), 
	RotationY(0.0f),*/ firstUpdate(true)
{
	if (CursorControl)
		CursorControl->grab();

	MoveSpeed /= 1000.0f;

	recalculateViewArea();

	allKeysUp();

	// create key map
	if (!keyMapArray || !keyMapSize)
	{
		// create default key map
		KeyMap.push_back(SCamKeyMap(0, ox::KEY_UP));
		KeyMap.push_back(SCamKeyMap(1, ox::KEY_DOWN));
		KeyMap.push_back(SCamKeyMap(2, ox::KEY_LEFT));
		KeyMap.push_back(SCamKeyMap(3, ox::KEY_RIGHT));
	}
	else
	{
		// create custom key map

		for (int i=0; i<keyMapSize; ++i)
		{
			switch(keyMapArray[i].Action)
			{
			case EKA_MOVE_FORWARD: KeyMap.push_back(SCamKeyMap(0, keyMapArray[i].KeyCode));
				break;
			case EKA_MOVE_BACKWARD: KeyMap.push_back(SCamKeyMap(1, keyMapArray[i].KeyCode));
				break;
			case EKA_STRAFE_LEFT: KeyMap.push_back(SCamKeyMap(2, keyMapArray[i].KeyCode));
				break;
			case EKA_STRAFE_RIGHT: KeyMap.push_back(SCamKeyMap(3, keyMapArray[i].KeyCode));
				break;
			} // end switch
		} // end for
	}// end if
}


//! destructor
CCameraFPSSceneNode::~CCameraFPSSceneNode()
{
	if (CursorControl)
		CursorControl->drop();
}


//! It is possible to send mouse and key events to the camera. Most cameras
//! may ignore this input, but camera scene nodes which are created for 
//! example with ox::scene::ISceneManager::addMayaCameraSceneNode or
//! ox::scene::ISceneManager::addMeshViewerCameraSceneNode, may want to get this input
//! for changing their position, look at target or whatever. 
bool CCameraFPSSceneNode::OnEvent(const ox::event::SEvent& event)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}



//! onPostRender is called just after rendering the whole scene.
//! nodes may calculate or store animations here, and may do other useful things,
//! dependent on what they are.
void CCameraFPSSceneNode::OnPostRender(unsigned int timeMs)
{
	animate();

	ox::TList<ISceneNodeAnimator*>::iterator ait = Animators.begin();
				for (; ait != Animators.end(); ++ait)
					(*ait)->animateNode(this, timeMs);

	updateAbsolutePosition();
	Target = getPosition() + TargetVector;

	ox::TList<ISceneNode*>::iterator it = Children.begin();
				for (; it != Children.end(); ++it)
					(*it)->OnPostRender(timeMs);
}



void CCameraFPSSceneNode::animate()
{
	if (SceneManager->getActiveCamera() != this)
		return;

	if (firstUpdate)
	{
		if (CursorControl)
			CursorControl->setPosition(0.5f, 0.5f);

		LastAnimationTime = os::Timer::getTime();

		firstUpdate = false;
		return;
	}

	// get time

	int now = os::Timer::getTime();
	int timeDiff = now - LastAnimationTime;
	LastAnimationTime = now;

	// Update rotation

	Target.set(0,0,1);

	if (!CursorControl)
		return;

	RelativeRotation.X *= -1.0f;
	RelativeRotation.Y *= -1.0f;

	if (InputReceiverEnabled)
	{
		ox::core::CPosition2d<float> cursorpos = CursorControl->getRelativePosition();

		if (cursorpos.X < 0.49f || cursorpos.X > 0.51f ||
			cursorpos.Y < 0.49f || cursorpos.Y > 0.51f)
		{
			RelativeRotation.Y += (0.5f - cursorpos.X) * RotateSpeed;
			RelativeRotation.X += (0.5f - cursorpos.Y) * RotateSpeed;
			CursorControl->setPosition(0.5f, 0.5f);

			if (RelativeRotation.X > 89.0f) RelativeRotation.X = 89.0f;
			if (RelativeRotation.X < -89.0f) RelativeRotation.X = -89.0f;
		}
	}

	// set target

	ox::core::CMatrix4 mat;
	mat.setRotationDegrees(ox::core::CVector3d<float>(-RelativeRotation.X, -RelativeRotation.Y, 0));
	mat.transformVect(Target);

	// update position

	ox::core::CVector3d<float> pos = getPosition();	

	ox::core::CVector3d<float> movedir = Target;
	movedir.normalize();

	if (CursorKeys[0])
		pos += movedir * (float)timeDiff * MoveSpeed;

	if (CursorKeys[1])
		pos -= movedir * (float)timeDiff * MoveSpeed;

	// strafing

	ox::core::CVector3d<float> strafevect = Target;
	strafevect = strafevect.crossProduct(UpVector);
	strafevect.normalize();

	if (CursorKeys[2])
		pos += strafevect * (float)timeDiff * MoveSpeed;

	if (CursorKeys[3])
		pos -= strafevect * (float)timeDiff * MoveSpeed;

	// write translation

	setPosition(pos);

	// write right target

	TargetVector = Target;
	Target += pos;

	RelativeRotation.X *= -1.0f;
	RelativeRotation.Y *= -1.0f;
}


void CCameraFPSSceneNode::allKeysUp()
{
	for (int i=0; i<4; ++i)
		CursorKeys[i] = false;
}


//! sets the look at target of the camera
//! \param pos: Look at target of the camera.
void CCameraFPSSceneNode::setTarget(const ox::core::CVector3d<float>& wtgt)
{
	// calculate rotation to fit new target
	ox::core::CVector3d<float> cpos = getAbsolutePosition();
	
	ox::core::CVector2d<float> wline(cpos.Z-wtgt.Z, cpos.X-wtgt.X);
	wline.normalize();
	float wangle = (float)wline.getAngle();
    RelativeRotation.Y = wangle + 180;

	// This part of the function is quite weired.

	wline.set(cpos.X-wtgt.X, cpos.Y-wtgt.Y);
	wline.normalize();
	wangle =  (float)wline.getAngle();
	RelativeRotation.X =  180 - wangle;

	// I don't remember why I wrote this, but
	// it works partially. There is absolutely
	// no need for a while loop. I'll rewrite it 
	// when I am in the mood...

	int cnt = 0;
	while((RelativeRotation.X < -90.0f || RelativeRotation.X > 90.0f) && cnt<10)
	{
		if (RelativeRotation.X < -90.0f)
			RelativeRotation.X = -(RelativeRotation.X + 180.0f);
		if (RelativeRotation.X > 90.0f)
			RelativeRotation.X = -(RelativeRotation.X - 180.0f);

		++cnt;
	}
}


} // end namespace
} // end namespace

