// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CCameraMayaSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CDimension2d.h"
#include "ox/core/CVector3d.h"
#include "ox/core/irrMath.h"
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

	ISceneNode::setPosition(Pos);
	updateAbsolutePosition();

	// This scene node cannot be animated by scene node animators.
}


bool CCameraMayaSceneNode::isMouseKeyDown(int key)
{
	return MouseKeys[key];
}



void CCameraMayaSceneNode::animate()
{
	//Alt + LM = Rotieren um KameraDrehpunkt
	//Alt + LM + MM = Dolly vor/zurück in Kamerablickrichtung (Geschwindigkeit % von Abstand Kamera zu Drehpunkt - Maximalweg zum Kameradrehpunkt)
	//Alt + MM = Verschieben in Kameraebene (Bildmitte hängt vom Tempo ungefähr am Mauszeiger)

	const SViewFrustrum* va = getViewFrustrum();

	float nRotX = rotX;
	float nRotY = rotY;
	float nZoom = currentZoom;

	if (isMouseKeyDown(0) && isMouseKeyDown(2))
	{
		if (!zooming)
		{
			zoomStartX = MousePos.X;
			zoomStartY = MousePos.Y;
			zooming = true;
			nZoom = currentZoom;
		}
		else
		{
			float old = nZoom;
			nZoom += (zoomStartX - MousePos.X) * zoomSpeed;

			if (nZoom < 0)
				nZoom = old;
		}
	}
	else
	{
		if (zooming)
		{
			float old = currentZoom;
			currentZoom = currentZoom + (zoomStartX - MousePos.X ) * zoomSpeed;
			nZoom = currentZoom;

			if (nZoom < 0)
				nZoom = currentZoom = old;
		}

		zooming = false;
	}

	// Translation ---------------------------------

	ox::core::CVector3d<float> translate(oldTargetX, oldTargetY, oldTargetZ);

	ox::core::CVector3d<float> tvectX = Pos - Target;
	tvectX = tvectX.crossProduct(UpVector);
	tvectX.normalize();

	ox::core::CVector3d<float> tvectY = (va->getFarLeftDown() - va->getFarRightDown());
	tvectY = tvectY.crossProduct(UpVector.Y > 0 ? Pos - Target : Target - Pos);
	tvectY.normalize();


	if (isMouseKeyDown(2) && !zooming)
	{
		if (!translating)
		{
			translateStartX = MousePos.X;
			translateStartY = MousePos.Y;
			translating = true;
		}
		else
		{
			translate +=	tvectX * (translateStartX - MousePos.X)*translateSpeed +
								tvectY * (translateStartY - MousePos.Y)*translateSpeed;
		}
	}
	else
	{
		if (translating)
		{
			translate +=	tvectX * (translateStartX - MousePos.X)*translateSpeed +
								tvectY * (translateStartY - MousePos.Y)*translateSpeed;
			oldTargetX = translate.X;
			oldTargetY = translate.Y;
			oldTargetZ = translate.Z;
		}

		translating = false;
	}

	// Rotation ------------------------------------

	if (isMouseKeyDown(0) && !zooming)
	{
		if (!rotating)
		{
			rotateStartX = MousePos.X;
			rotateStartY = MousePos.Y;
			rotating = true;
			nRotX = rotX;
			nRotY = rotY;
		}
		else
		{
			nRotX += (rotateStartX - MousePos.X) * rotateSpeed;
			nRotY += (rotateStartY - MousePos.Y) * rotateSpeed;
		}
	}
	else
	{
		if (rotating)
		{
			rotX = rotX + (rotateStartX - MousePos.X) * rotateSpeed;
			rotY = rotY + (rotateStartY - MousePos.Y) * rotateSpeed;
			nRotX = rotX;
			nRotY = rotY;
		}

		rotating = false;
	}

	// Set Pos ------------------------------------

	Target = translate;

	Pos.X = nZoom + Target.X;
	Pos.Y = Target.Y;
	Pos.Z = Target.Z;

	{ double deg = (nRotY) * 0.017453290522098541; float cs = (float)cos(deg); float sn = (float)sin(deg); Pos.X -= (Target).X; Pos.Y -= (Target).Y; Pos.set(Pos.X*cs - Pos.Y*sn, Pos.X*sn + Pos.Y*cs, Pos.Z); Pos.X += (Target).X; Pos.Y += (Target).Y; }
	{ double deg = (-nRotX) * 0.017453290522098541; float cs = (float)cos(deg); float sn = (float)sin(deg); Pos.X -= (Target).X; Pos.Z -= (Target).Z; Pos.set(Pos.X*cs - Pos.Z*sn, Pos.Y, Pos.X*sn + Pos.Z*cs); Pos.X += (Target).X; Pos.Z += (Target).Z; }

	// Rotation Error ----------------------------

	if (nRotY < 0.0f)
		nRotY *= -1.0f;

	nRotY = (float)fmod(nRotY, 360.0f);


	if (nRotY >= 90.0f && nRotY <= 270.0f)
		UpVector.set(0, -1, 0);
	else
		UpVector.set(0, 1, 0);
}


void CCameraMayaSceneNode::allKeysUp()
{
	for (int i=0; i<3; ++i)
		MouseKeys[i] = false;
}


//! sets the position of the node
void CCameraMayaSceneNode::setPosition(const ox::core::CVector3d<float>& newpos)
{
	currentZoom = newpos.getLength();
}

} // end namespace
} // end namespace
