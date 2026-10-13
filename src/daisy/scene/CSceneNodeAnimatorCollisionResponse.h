// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneNodeAnimatorCollisionResponse.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_SCENE_NODE_ANIMATOR_COLLISION_RESPONSE_H_INCLUDED__
#define __C_SCENE_NODE_ANIMATOR_COLLISION_RESPONSE_H_INCLUDED__

#include "ox/scene/ISceneNodeAnimatorCollisionResponse.h"
#include "ox/core/CTriangle3d.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/ITriangleSelector.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	//! Special scene node animator for doing automatic collision detection and response.
	/** This scene node animator can be attached to any scene node modifying it in that
	way, that it cannot move through walls of the world, is influenced by gravity and
	acceleration. This animator is useful for example for first person shooter 
	games. Attach it for example to a first person shooter camera, and the camera will
	behave as the player control in a first person shooter game: The camera stops and
	slides at walls, walks up stairs, falls down if there is no floor under it, and so on.
	*/
	class CSceneNodeAnimatorCollisionResponse : public ISceneNodeAnimatorCollisionResponse
	{
	public:

		//! constructor
		CSceneNodeAnimatorCollisionResponse(ISceneManager* scenemanager,
			ITriangleSelector* world, ISceneNode* object, 
			const ox::core::CVector3d<float>& ellipsoidRadius = ox::core::CVector3d<float>(30,60,30),
			const ox::core::CVector3d<float>& gravityPerSecond = ox::core::CVector3d<float>(0,-100.0f,0),
			float accelerationPerSecond=100.0f,
			const ox::core::CVector3d<float>& ellipsoidTranslation = ox::core::CVector3d<float>(0,0,0),
			float slidingSpeed = 0.0005f);

		//! destructor
		virtual ~CSceneNodeAnimatorCollisionResponse();

		//! Returns if the attached scene node is falling, which means that
		//! there is no blocking wall from the scene node in the direction of
		//! the gravity.
		virtual bool isFalling();

		//! Sets the radius of the ellipsoid with which collision detection and
		//! response is done. 
		virtual void setEllipsoidRadius(const ox::core::CVector3d<float>& radius);

		//! Returns the radius of the ellipsoid with wich the collision detection and
		//! response is done.
		virtual ox::core::CVector3d<float> getEllipsoidRadius() const;

		//! Sets the gravity of the environment.
		virtual void setGravity(const ox::core::CVector3d<float>& gravity);

		//! Returns current vector of gravity.
		virtual ox::core::CVector3d<float> getGravity() const;

		//! Sets the translation of the ellipsoid for collision detection.
		virtual void setEllipsoidTranslation(ox::core::CVector3d<float> translation);

		//! Returns the translation of the ellipsoid for collision detection.
		virtual ox::core::CVector3d<float> getEllipsoidTranslation() const;

		//! Returns the acceleration per second, if the scene node is falling down.
		virtual float getAccelerationPerSecond() const;

		//! Sets a new acceleration per second value.
		virtual void setAccelerationPerSecond(float accel);

		//! Sets a triangle selector holding all triangles of the world with which
		//! the scene node may collide.
		virtual void setWorld(ITriangleSelector* newWorld);

		//! Returns the current triangle selector containing all triangles for
		//! collision detection.
		virtual ITriangleSelector* getWorld() const;

		//! animates a scene node
		virtual void animateNode(ISceneNode* node, unsigned int timeMs);

	private:

		ox::core::CVector3d<float> LastPosition;
		ox::core::CVector3d<float> Radius;
		ox::core::CVector3d<float> Gravity;
		ox::core::CVector3d<float> Translation;
		
		ITriangleSelector* World;
		ISceneNode* Object;
		ISceneManager* SceneManager;
		unsigned int LastTime;
		unsigned int FallStartTime;
		float AccelerationPerSecond;
		float SlidingSpeed;
		bool Falling;

		ox::core::CTriangle3d<float> RefTriangle;
	};

} // end namespace scene
} // end namespace daisy

#endif

