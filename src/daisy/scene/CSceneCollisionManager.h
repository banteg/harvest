#ifndef __C_SCENE_COLLISION_MANAGER_H_INCLUDED__
#define __C_SCENE_COLLISION_MANAGER_H_INCLUDED__

// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneCollisionManager.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include "ox/scene/ISceneCollisionManager.h"
#include "ox/core/CLine3d.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CTriangle3d.h"
#include "ox/core/CVector3d.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/ITriangleSelector.h"
#include "ox/TArray.h"
#include "ox/scene/ISceneManager.h"
#include "ox/video/IVideoDriver.h"
#include <math.h>

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	//!	The Scene Collision Manager provides methods for performing collision tests and picking on scene nodes.
	class CSceneCollisionManager : public ISceneCollisionManager
	{
	public:

		//! constructor
		CSceneCollisionManager(ISceneManager* smanager, ox::video::IVideoDriver* driver);

		//! destructor
		virtual ~CSceneCollisionManager();

		//! Returns the scene node, which is currently visible under the overgiven 
		//! screencoordinates, viewed from the currently active camera. 
		virtual ISceneNode* getSceneNodeFromScreenCoordinatesBB(ox::core::CPosition2d<int> pos,
			int idBitMask=0);

		//! Returns the nearest scene node which collides with a 3d ray and 
		//! which id matches a bitmask. 
		virtual ISceneNode* getSceneNodeFromRayBB(ox::core::CLine3d<float> ray, int idBitMask=0);

		//! Returns the scene node, at which the overgiven camera is looking at and
		//! which id matches the bitmask.
		virtual ISceneNode* getSceneNodeFromCameraBB(ICameraSceneNode* camera, int idBitMask=0);

		//! Finds the collision point of a line and lots of triangles, if there is one.
		virtual bool getCollisionPoint(const ox::core::CLine3d<float>& ray,
			ITriangleSelector* selector, ox::core::CVector3d<float>& outCollisionPoint,
			ox::core::CTriangle3d<float>& outTriangle);

		//! Collides a moving ellipsoid with a 3d world with gravity and returns
		//! the resulting new position of the ellipsoid. 
		virtual ox::core::CVector3d<float> getCollisionResultPosition(
			ITriangleSelector* selector,
			const ox::core::CVector3d<float> &ellipsoidPosition,	const ox::core::CVector3d<float>& ellipsoidRadius, 
			const ox::core::CVector3d<float>& ellipsoidDirectionAndSpeed,
			ox::core::CTriangle3d<float>& triout,
			bool& outFalling,
			float slidingSpeed,
			const ox::core::CVector3d<float>& gravityDirectionAndSpeed);

		//! Returns a 3d ray which would go through the 2d screen coodinates.
		virtual ox::core::CLine3d<float> getRayFromScreenCoordinates(
			ox::core::CPosition2d<int> pos, ICameraSceneNode* camera = 0);

		//! Calculates 2d screen position from a 3d position.
		virtual ox::core::CPosition2d<int> getScreenCoordinatesFrom3DPosition(
			ox::core::CVector3d<float> pos, ICameraSceneNode* camera=0);

	private:

		//! recursive method for going through all scene nodes
		void getPickedNodeBB(ISceneNode* root, const ox::core::CVector3d<float>& linemiddle, 
				const ox::core::CVector3d<float>& linevect,
				const ox::core::CVector3d<float>& pos, 
				float halflength, int bits, float& outdist,
				ISceneNode*& outbest);

		struct SCollisionData
		{
			ox::core::CVector3d<float> eRadius;

			ox::core::CVector3d<float> R3Velocity; 
			ox::core::CVector3d<float> R3Position;

			ox::core::CVector3d<float> velocity;
			ox::core::CVector3d<float> normalizedVelocity;
			ox::core::CVector3d<float> basePoint;

			bool foundCollision;
			double nearestDistance;
			ox::core::CVector3d<float> intersectionPoint;

			ox::core::CTriangle3d<float> intersectionTriangle;
			int triangleHits;

			float slidingSpeed;

			ITriangleSelector* selector;
		};

		void testTriangleIntersection(SCollisionData* colData, 
			const ox::core::CTriangle3d<float>& triangle);

		//! recursive method for doing collision response
		ox::core::CVector3d<float> collideEllipsoidWithWorld(ITriangleSelector* selector, 
			const ox::core::CVector3d<float> &position,
			const ox::core::CVector3d<float>& radius,  const ox::core::CVector3d<float>& velocity,
			float slidingSpeed,
			const ox::core::CVector3d<float>& gravity, ox::core::CTriangle3d<float>& triout,
			bool& outFalling);

		ox::core::CVector3d<float> collideWithWorld(int recursionDepth, SCollisionData &colData,
			ox::core::CVector3d<float> pos, ox::core::CVector3d<float> vel);

		inline bool getLowestRoot(float a, float b, float c, float maxR, float* root)
		{
			// check if solution exists
			float determinant = b*b - 4.0f*a*c;

			// if determinant is negative, no solution
			if (determinant < 0.0f) return false;

			// calculate two roots: (if det==0 then x1==x2
			// but lets disregard that slight optimization)

			float sqrtD = (float)sqrt(determinant);
			float r1 = (-b - sqrtD) / (2*a);
			float r2 = (-b + sqrtD) / (2*a);

			// sort so x1 <= x2
			if (r1 > r2) { float tmp=r2; r2=r1; r1=tmp; }

			// get lowest root
			if (r1 > 0 && r1 < maxR)
			{
				*root = r1;
				return true;
			}

			// its possible that we want x2, this can happen if x1 < 0
			if (r2 > 0 && r2 < maxR)
			{
				*root = r2;
				return true;
			}

			return false;
		}

		ISceneManager* SceneManager;
		ox::video::IVideoDriver* Driver;
		ox::TArray<ox::core::CTriangle3d<float> > Triangles; // triangle buffer 
	};


} // end namespace scene
} // end namespace daisy

#endif

