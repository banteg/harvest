// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneNodeAnimatorCollisionResponse.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CTriangle3d.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/ITriangleSelector.h"
#include "ox/event/ILogger.h"
#include "CSceneNodeAnimatorCollisionResponse.h"
#include "ox/scene/ISceneCollisionManager.h"
#include "ox/scene/ISceneManager.h"
#include "daisy/os.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CSceneNodeAnimatorCollisionResponse::CSceneNodeAnimatorCollisionResponse(
		ISceneManager* scenemanager,
		ITriangleSelector* world, ISceneNode* object,
		const ox::core::CVector3d<float>& ellipsoidRadius,
		const ox::core::CVector3d<float>& gravityPerSecond,
		float accelerationPerSecond,
		const ox::core::CVector3d<float>& ellipsoidTranslation,
		float slidingSpeed)
: SceneManager(scenemanager), World(world), Object(object),
	Radius(ellipsoidRadius), Gravity(gravityPerSecond / 1000.0f),
	SlidingSpeed(slidingSpeed), Translation(ellipsoidTranslation)
{
	setAccelerationPerSecond(accelerationPerSecond);

	if (World)
		World->grab();

	if (Object)
		LastPosition = Object->getPosition();

	Falling = false;

	LastTime = os::Timer::getTime();
	FallStartTime = LastTime;

	RefTriangle.pointA.set(0.0f, 0.0f, 0.0f);
	RefTriangle.pointB.set(0.0f, 0.0f, 0.0f);
	RefTriangle.pointC.set(0.0f, 0.0f, 0.0f);
}



//! destructor
CSceneNodeAnimatorCollisionResponse::~CSceneNodeAnimatorCollisionResponse()
{
	if (World)
		World->drop();
}


//! Returns if the attached scene node is falling, which means that
//! there is no blocking wall from the scene node in the direction of
//! the gravity.
bool CSceneNodeAnimatorCollisionResponse::isFalling()
{
	return Falling;
}


//! Sets the radius of the ellipsoid with which collision detection and
//! response is done.
void CSceneNodeAnimatorCollisionResponse::setEllipsoidRadius(
	const ox::core::CVector3d<float>& radius)
{
	Radius = radius;
}

//! Returns the radius of the ellipsoid with wich the collision detection and
//! response is done.
ox::core::CVector3d<float> CSceneNodeAnimatorCollisionResponse::getEllipsoidRadius() const
{
	return Radius;
}


//! Sets the gravity of the environment.
void CSceneNodeAnimatorCollisionResponse::setGravity(const ox::core::CVector3d<float>& gravity)
{
	Gravity = gravity;
}


//! Returns current vector of gravity.
ox::core::CVector3d<float> CSceneNodeAnimatorCollisionResponse::getGravity() const
{
	return Gravity;
}


//! Sets the translation of the ellipsoid for collision detection.
void CSceneNodeAnimatorCollisionResponse::setEllipsoidTranslation(ox::core::CVector3d<float> translation)
{
	Translation = translation;
}



//! Returns the translation of the ellipsoid for collision detection.
ox::core::CVector3d<float> CSceneNodeAnimatorCollisionResponse::getEllipsoidTranslation() const
{
	return Translation;
}



//! Returns the acceleration per second, if the scene node is falling down.
float CSceneNodeAnimatorCollisionResponse::getAccelerationPerSecond() const
{
	return AccelerationPerSecond * 1000000.0f;
}



//! Sets a new acceleration per second value.
void CSceneNodeAnimatorCollisionResponse::setAccelerationPerSecond(float accel)
{
	AccelerationPerSecond = accel / 1000000.0f;
}



//! Sets a triangle selector holding all triangles of the world with which
//! the scene node may collide.
void CSceneNodeAnimatorCollisionResponse::setWorld(ITriangleSelector* newWorld)
{
	if (World)
		World->drop();

	World = newWorld;
	if (World)
		World->grab();
}



//! Returns the current triangle selector containing all triangles for
//! collision detection.
ITriangleSelector* CSceneNodeAnimatorCollisionResponse::getWorld() const
{
	return World;
}



//! animates a scene node
void CSceneNodeAnimatorCollisionResponse::animateNode(ISceneNode* node, unsigned int timeMs)
{
	if (node != Object)
	{
		os::Printer::log("CollisionResponseAnimator only works with same scene node as set as object during creation", ox::event::ELL_ERROR);
		return;
	}

	if (!World)
		return;

	unsigned int diff = timeMs - LastTime;
	LastTime = timeMs;

	ox::core::CVector3d<float> pos = Object->getPosition();
	ox::core::CVector3d<float> vel = pos - LastPosition;
	ox::core::CVector3d<float> g = Gravity * (float)diff;

	if (Falling)
		g = g * (AccelerationPerSecond * (float)(timeMs - FallStartTime)*(timeMs - FallStartTime));

	ox::core::CTriangle3d<float> triangle = RefTriangle;

	if (vel+g != ox::core::CVector3d<float>(0,0,0))
	{
		// TODO: divide SlidingSpeed by frame time

		bool f = false;
		pos = SceneManager->getSceneCollisionManager()->getCollisionResultPosition(
				World, LastPosition-Translation,
				Radius, vel, triangle, f, SlidingSpeed, g);

		pos += Translation;

		if (f)//triangle == RefTriangle)
		{
			if (!Falling)
				FallStartTime = timeMs;

			Falling = true;
		}
		else
			Falling = false;

		Object->setPosition(pos);
	}

	LastPosition = Object->getPosition();
}



} // end namespace scene
} // end namespace daisy
