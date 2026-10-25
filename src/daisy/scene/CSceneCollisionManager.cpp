#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CLine3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CPlane3d.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CTriangle3d.h"
#include "ox/core/CVector3d.h"
#include "ox/video/IVideoDriver.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/SViewFrustrum.h"
#include "ox/TList.h"
#include "CSceneCollisionManager.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/scene/ITriangleSelector.h"

#include "daisy/os.h"
#include <stdio.h>

inline bool isOnSameSide(const ox::core::CVector3d<float>& p1, const ox::core::CVector3d<float>& p2,
		const ox::core::CVector3d<float>& a, const ox::core::CVector3d<float>& b)
{
	ox::core::CVector3d<float> bminusa = b - a;
	ox::core::CVector3d<float> cp1 = bminusa.crossProduct(p1 - a);
	ox::core::CVector3d<float> cp2 = bminusa.crossProduct(p2 - a);
	return (cp1.dotProduct(cp2) > 0.0f);
}

inline bool getIntersectionWithLine(const ox::core::CTriangle3d<float>& triangle,
		const ox::core::CVector3d<float>& linePoint, const ox::core::CVector3d<float>& lineVect,
		ox::core::CVector3d<float>& outIntersection)
{
	ox::core::CVector3d<float> normal = (triangle.pointB - triangle.pointA).crossProduct(triangle.pointC - triangle.pointA);
	float t2 = normal.dotProduct(lineVect);
	if (t2 == 0.0f)
		return false;
	float d = triangle.pointA.dotProduct(normal);
	float t = -(normal.dotProduct(linePoint) - d) / t2;
	outIntersection = linePoint + (lineVect * t);
	return (isOnSameSide(outIntersection, triangle.pointA, triangle.pointB, triangle.pointC) &&
		isOnSameSide(outIntersection, triangle.pointB, triangle.pointA, triangle.pointC) &&
		isOnSameSide(outIntersection, triangle.pointC, triangle.pointA, triangle.pointB));
}

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CSceneCollisionManager::CSceneCollisionManager(ISceneManager* smanager, ox::video::IVideoDriver* driver)
: Driver(driver), SceneManager(smanager)
{

	if (Driver)
		Driver->grab();
}



//! destructor
CSceneCollisionManager::~CSceneCollisionManager()
{
	if (Driver)
		Driver->drop();
}



//! Returns the scene node, which is currently visible under the overgiven
//! screencoordinates, viewed from the currently active camera.
ISceneNode* CSceneCollisionManager::getSceneNodeFromScreenCoordinatesBB(
	ox::core::CPosition2d<int> pos, int idBitMask)
{
	if (!SceneManager || !Driver)
		return 0;

	ICameraSceneNode* camera = SceneManager->getActiveCamera();
	if (!camera)
		return 0;

	const ox::scene::SViewFrustrum* f = camera->getViewFrustrum();

	ox::core::CVector3d<float> farLeftUp = f->getFarLeftUp();
	ox::core::CVector3d<float> lefttoright = f->getFarRightUp() - farLeftUp;
	ox::core::CVector3d<float> uptodown = f->getFarLeftDown() - farLeftUp;

	ox::core::CDimension2d<int> screenSize = Driver->getScreenSize();

	float dx = pos.X / (float)screenSize.Width;
	float dy = pos.Y / (float)screenSize.Height;

	ox::core::CVector3d<float> end = farLeftUp + (lefttoright * dx) + (uptodown * dy);

	return getSceneNodeFromRayBB(ox::core::CLine3d<float>(f->cameraPosition, end), idBitMask);
}



//! Returns the nearest scene node which collides with a 3d ray and
//! which id matches a bitmask.
ISceneNode* CSceneCollisionManager::getSceneNodeFromRayBB(ox::core::CLine3d<float> ray,
														int idBitMask)
{
	ISceneNode* best = 0;
	float dist = 9999999999.0f;

	getPickedNodeBB(SceneManager->getRootSceneNode(), ray.getMiddle(),
		ray.getVector().normalize(), ray.start, (float)(ray.getLength() * 0.5),
		idBitMask, dist, best);

	return best;
}


//! recursive method for going through all scene nodes
void CSceneCollisionManager::getPickedNodeBB(ISceneNode* root,
									   const ox::core::CVector3d<float>& linemiddle,
									   const ox::core::CVector3d<float>& linevect,
									   const ox::core::CVector3d<float>& pos,
									   float halflength, int bits,
									   float& outbestdistance,
									   ISceneNode*& outbestnode)
{
	const ox::TList<ISceneNode*>& children = root->getChildren();

	ox::TList<ISceneNode*>::const_iterator it = children.begin();

	for (; it != children.end(); ++it)
	{
		ISceneNode* current = *it;

		if (current->isVisible() &&
			(bits==0 || (bits != 0 && (current->getID() & bits))))
		{
			if (current->getTransformedBoundingBox().intersectsWithLine(
				linemiddle, linevect, halflength))
			{
				float dist = 	(float)current->getAbsolutePosition().getDistanceFrom(pos);
				if (dist < outbestdistance)
				{
					outbestnode = current;
					outbestdistance = dist;
				}
			}
		}

		getPickedNodeBB(current, linemiddle, linevect, pos,
			halflength, bits, outbestdistance, outbestnode);
	}
}



//! Returns the scene node, at which the overgiven camera is looking at and
//! which id matches the bitmask.
ISceneNode* CSceneCollisionManager::getSceneNodeFromCameraBB(
	ICameraSceneNode* camera, int idBitMask)
{
	if (!camera)
		return 0;

	ox::core::CVector3d<float> start = camera->getAbsolutePosition();
	ox::core::CVector3d<float> end = camera->getTarget();

	end = start + ((end - start).normalize() * camera->getFarValue());
	ox::core::CLine3d<float> line(start, end);

	return getSceneNodeFromRayBB(line, idBitMask);
}



//! Finds the collision point of a line and lots of triangles, if there is one.
bool CSceneCollisionManager::getCollisionPoint(const ox::core::CLine3d<float>& ray,
	ITriangleSelector* selector, ox::core::CVector3d<float>& outIntersection,
	ox::core::CTriangle3d<float>& outTriangle)
{
	if (!selector)
		return false;

	int totalcnt = selector->getTriangleCount();
	Triangles.resize(totalcnt);

	int cnt = 0;
	selector->getTriangles(&Triangles[0], totalcnt, cnt, ray);

	ox::core::CVector3d<float> linevect = ray.getVector();
	ox::core::CVector3d<float> intersection;
	float nearest = 9999999999999.0f;
	bool found = false;
	float tmp, tmp2;
	float raylenght = (float)ray.getLengthSQ();

	for (int i=0; i<cnt; ++i)
	{
		if (getIntersectionWithLine(Triangles[i], ray.start, linevect, intersection))
		{
			tmp = (float)intersection.getDistanceFromSQ(ray.start);
			tmp2 = (float)intersection.getDistanceFromSQ(ray.end);

            if (tmp < raylenght && tmp2 < raylenght &&
				tmp < nearest)
			{
				nearest = tmp;
				outTriangle = Triangles[i];
				outIntersection = intersection;
				found = true;
			}
		}
	}

	return found;
}



//! Collides a moving ellipsoid with a 3d world with gravity and returns
//! the resulting new position of the ellipsoid.
ox::core::CVector3d<float> CSceneCollisionManager::getCollisionResultPosition(
	ITriangleSelector* selector,
	const ox::core::CVector3d<float> &position,	const ox::core::CVector3d<float>& radius,
	const ox::core::CVector3d<float>& direction,
	ox::core::CTriangle3d<float>& triout,
	bool& outFalling,
	float slidingSpeed,
	const ox::core::CVector3d<float>& gravity)
{
	if (!selector || radius.X == 0.0f || radius.Y == 0.0f || radius.Z == 0.0f)
		return position;

	return collideEllipsoidWithWorld(selector, position,
		radius, direction, slidingSpeed, gravity, triout, outFalling);
}


void CSceneCollisionManager::testTriangleIntersection(SCollisionData* colData,
			const ox::core::CTriangle3d<float>& triangle)
{
	ox::core::CPlane3d<float> trianglePlane = triangle.getPlane();

	// only check front facing polygons
	if (trianglePlane.isFrontFacting(colData->normalizedVelocity))
	{
		// get interval of plane intersection

		double t1, t0;
		bool embeddedInPlane = false;

		// calculate signed distance from sphere position to triangle plane
		double signedDistToTrianglePlane = trianglePlane.getDistanceTo(
			colData->basePoint);

		float normalDotVelocity =
			trianglePlane.Normal.dotProduct(colData->velocity);

		if (normalDotVelocity == 0.0f)
		{
			// sphere is traveling paralell to plane

			if (fabs(signedDistToTrianglePlane) >= 1.0f)
				return; // no collision possible
			else
			{
				// sphere is embedded in plane
				embeddedInPlane = true;
				t0 = 0.0;
				t1 = 1.0;
			}
		}
		else
		{
			// N.D is not 0. Calculate intersection interval
			t0 = (-1.0-signedDistToTrianglePlane)/normalDotVelocity;
			t1 = (1.0-signedDistToTrianglePlane)/normalDotVelocity;

			// Swap so t0 < t1
			if (t0 > t1) { double tmp = t1; t1 = t0; t0 = tmp;	}

			// check if at least one value is within the range
			if (t0 > 1.0f || t1 < 0.0f)
				return; // both t values are outside 1 and 0, no collision possible

			// clamp to 0 and 1
			if (t0 < 0.0) t0 = 0.0;
			if (t1 < 0.0) t1 = 0.0;
			if (t0 > 1.0) t0 = 1.0;
			if (t1 > 1.0) t1 = 1.0;
		}

		// at this point we have t0 and t1, if there is any intersection, it
		// is between this interval

		ox::core::CVector3d<float> collisionPoint;
		bool foundCollision = false;
		float t = 1.0f;

		// first check the easy case: Collision within the triangle;
		// if this happens, it must be at t0 and this is when the sphere
		// rests on the front side of the triangle plane. This can only happen
		// if the sphere is not embedded in the triangle plane.

		if (!embeddedInPlane)
		{
			ox::core::CVector3d<float> planeIntersectionPoint =
				(colData->basePoint - trianglePlane.Normal)
				+ (colData->velocity * (float)t0);

			if (triangle.isPointInsideFast(planeIntersectionPoint))
			{
				foundCollision = true;
				t = (float)t0;
				collisionPoint = planeIntersectionPoint;
			}
		}

		// if we havent found a collision already we will have to sweep
		// the sphere against points and edges of the triangle. Note: A
		// collision inside the triangle will always happen before a
		// vertex or edge collision.

		if (!foundCollision)
		{
			ox::core::CVector3d<float> velocity = colData->velocity;
			ox::core::CVector3d<float> base = colData->basePoint;

			float velocitySqaredLength = (float)velocity.getLengthSQ();
			float a,b,c;
			float newT;

			// for each edge or vertex a quadratic equation has to be solved:
			// a*t^2 + b*t + c = 0. We calculate a,b, and c for each test.

			// check against points
			a = velocitySqaredLength;

			// p1
			b = 2.0f * (velocity.dotProduct(base - triangle.pointA));
			c = (float)((triangle.pointA-base).getLengthSQ() - 1.0);
			if (getLowestRoot(a,b,c,t, &newT))
			{
				t = newT;
				foundCollision = true;
				collisionPoint = triangle.pointA;
			}

			// p2
			if (!foundCollision)
			{
				b = 2.0f * (velocity.dotProduct(base - triangle.pointB));
				c = (float)((triangle.pointB-base).getLengthSQ() - 1.0);
				if (getLowestRoot(a,b,c,t, &newT))
				{
					t = newT;
					foundCollision = true;
					collisionPoint = triangle.pointB;
				}
			}

			// p3
			if (!foundCollision)
			{
				b = 2.0f * (velocity.dotProduct(base - triangle.pointC));
				c = (float)((triangle.pointC-base).getLengthSQ() - 1.0);
				if (getLowestRoot(a,b,c,t, &newT))
				{
					t = newT;
					foundCollision = true;
					collisionPoint = triangle.pointC;
				}
			}

			// check against edges:

			// p1 --- p2
			ox::core::CVector3d<float> edge = triangle.pointB - triangle.pointA;
			ox::core::CVector3d<float> baseToVertex = triangle.pointA - base;
			float edgeSqaredLength = (float)edge.getLengthSQ();
			float edgeDotVelocity = edge.dotProduct(velocity);
			float edgeDotBaseToVertex = edge.dotProduct(baseToVertex);

			// calculate parameters for equation
			a = edgeSqaredLength* -velocitySqaredLength +
				edgeDotVelocity*edgeDotVelocity;
			b = edgeSqaredLength* (2*velocity.dotProduct(baseToVertex)) -
				2.0f*edgeDotVelocity*edgeDotBaseToVertex;
			c = (float)(edgeSqaredLength* (1-baseToVertex.getLengthSQ()) +
				edgeDotBaseToVertex*edgeDotBaseToVertex);

			// does the swept sphere collide against ininite edge?
			if (getLowestRoot(a,b,c,t,&newT))
			{
				float f = (edgeDotVelocity*newT - edgeDotBaseToVertex) / edgeSqaredLength;
				if (f >=0.0f && f <= 1.0f)
				{
					// intersection took place within segment
					t = newT;
					foundCollision = true;
					collisionPoint = triangle.pointA + (edge*f);
				}
			}

			// p2 --- p3
			edge = triangle.pointC-triangle.pointB;
			baseToVertex = triangle.pointB - base;
			edgeSqaredLength = (float)edge.getLengthSQ();
			edgeDotVelocity = edge.dotProduct(velocity);
			edgeDotBaseToVertex = edge.dotProduct(baseToVertex);

			// calculate parameters for equation
			a = edgeSqaredLength* -velocitySqaredLength +
				edgeDotVelocity*edgeDotVelocity;
			b = edgeSqaredLength* (2*velocity.dotProduct(baseToVertex)) -
				2.0f*edgeDotVelocity*edgeDotBaseToVertex;
			c = (float)(edgeSqaredLength* (1-baseToVertex.getLengthSQ()) +
				edgeDotBaseToVertex*edgeDotBaseToVertex);

			// does the swept sphere collide against ininite edge?
			if (getLowestRoot(a,b,c,t,&newT))
			{
				float f = (edgeDotVelocity*newT-edgeDotBaseToVertex) /
					edgeSqaredLength;
				if (f >=0.0f && f <= 1.0f)
				{
					// intersection took place within segment
					t = newT;
					foundCollision = true;
					collisionPoint = triangle.pointB + (edge*f);
				}
			}


			// p3 --- p1
			edge = triangle.pointA-triangle.pointC;
			baseToVertex = triangle.pointC - base;
			edgeSqaredLength = (float)edge.getLengthSQ();
			edgeDotVelocity = edge.dotProduct(velocity);
			edgeDotBaseToVertex = edge.dotProduct(baseToVertex);

			// calculate parameters for equation
			a = edgeSqaredLength* -velocitySqaredLength +
				edgeDotVelocity*edgeDotVelocity;
			b = edgeSqaredLength* (2*velocity.dotProduct(baseToVertex)) -
				2.0f*edgeDotVelocity*edgeDotBaseToVertex;
			c = (float)(edgeSqaredLength* (1-baseToVertex.getLengthSQ()) +
				edgeDotBaseToVertex*edgeDotBaseToVertex);

			// does the swept sphere collide against ininite edge?
			if (getLowestRoot(a,b,c,t,&newT))
			{
				float f = (edgeDotVelocity*newT-edgeDotBaseToVertex) /
					edgeSqaredLength;
				if (f >=0.0f && f <= 1.0f)
				{
					// intersection took place within segment
					t = newT;
					foundCollision = true;
					collisionPoint = triangle.pointC + (edge*f);
				}
			}
		}// end no collision found

		// set result:
		if (foundCollision)
		{
			// distance to collision is t
			float distToCollision = (float)(t*colData->velocity.getLength());

			// does this triangle qualify for closest hit?
			if (!colData->foundCollision ||
				distToCollision	< colData->nearestDistance)
			{
				colData->nearestDistance = distToCollision;
				colData->intersectionPoint = collisionPoint;
				colData->foundCollision = true;
				colData->intersectionTriangle = triangle;
				++colData->triangleHits;
			}

		}// end found collision

	}// end if is front facing
}



//! Collides a moving ellipsoid with a 3d world with gravity and returns
//! the resulting new position of the ellipsoid.
ox::core::CVector3d<float> CSceneCollisionManager::collideEllipsoidWithWorld(
	ITriangleSelector* selector, const ox::core::CVector3d<float> &position,
	const ox::core::CVector3d<float>& radius,  const ox::core::CVector3d<float>& velocity,
	float slidingSpeed,
	const ox::core::CVector3d<float>& gravity,
	ox::core::CTriangle3d<float>& triout, bool& outFalling)
{
	if (!selector || radius.X == 0.0f || radius.Y == 0.0f || radius.Z == 0.0f)
		return position;

	// This code is based on the paper "Improved Collision detection and Response"
	// by Kasper Fauerby, but some parts are modified.

	SCollisionData colData;
	colData.R3Position = position;
	colData.R3Velocity = velocity;
	colData.eRadius = radius;
	colData.nearestDistance = 9999999999999.0f;
	colData.selector = selector;
	colData.slidingSpeed = slidingSpeed;
	colData.triangleHits = 0;

	ox::core::CVector3d<float> eSpacePosition = colData.R3Position / colData.eRadius;
	ox::core::CVector3d<float> eSpaceVelocity = colData.R3Velocity / colData.eRadius;

	// iterate until we have our final position

	ox::core::CVector3d<float> finalPos = collideWithWorld(
		0, colData, eSpacePosition, eSpaceVelocity);

	outFalling = false;

	// add gravity

	if (gravity != ox::core::CVector3d<float>(0,0,0))
	{
		colData.R3Position = finalPos * colData.eRadius;
		colData.R3Velocity = gravity;
		colData.triangleHits = 0;

		eSpaceVelocity = gravity/colData.eRadius;

		finalPos = collideWithWorld(0, colData,
			finalPos, eSpaceVelocity);

		outFalling = (colData.triangleHits == 0);
	}

	if (colData.triangleHits)
	{
		triout = colData.intersectionTriangle;
		triout.pointA *= colData.eRadius;
		triout.pointB *= colData.eRadius;
		triout.pointC *= colData.eRadius;
	}

	finalPos *= colData.eRadius;
	return finalPos;
}

ox::core::CVector3d<float> CSceneCollisionManager::collideWithWorld(int recursionDepth,
	SCollisionData &colData, ox::core::CVector3d<float> pos, ox::core::CVector3d<float> vel)
{
	float veryCloseDistance = colData.slidingSpeed;

	if (recursionDepth > 5)
		return pos;

	colData.velocity = vel;
	colData.normalizedVelocity = vel;
	colData.normalizedVelocity.normalize();
	colData.basePoint = pos;
	colData.foundCollision = false;
	colData.nearestDistance = 9999999999999.0f;

	//------------------ collide with world

	// get all triangles with which we might collide
	ox::core::CAabbox3d<float> box(colData.R3Position);
	box.addInternalPoint(colData.R3Position + colData.R3Velocity);
	box.MinEdge -= colData.eRadius;
	box.MaxEdge += colData.eRadius;

	int totalTriangleCnt = colData.selector->getTriangleCount();
	Triangles.resize(totalTriangleCnt);

	ox::core::CMatrix4 scaleMatrix;
	scaleMatrix.setScale(
		ox::core::CVector3d<float>(1.0f / colData.eRadius.X, 1.0f / colData.eRadius.Y, 1.0f / colData.eRadius.Z));

	int triangleCnt = 0;
	colData.selector->getTriangles(&Triangles[0], totalTriangleCnt, triangleCnt, box, &scaleMatrix);
	//colData.selector->getTriangles(&Triangles[0], totalTriangleCnt, triangleCnt, &scaleMatrix);

	for (int i=0; i<triangleCnt; ++i)
		testTriangleIntersection(&colData, Triangles[i]);

	//---------------- end collide with world

	if (!colData.foundCollision)
		return pos + vel;

	// original destination point
	ox::core::CVector3d<float> destinationPoint = pos + vel;
	ox::core::CVector3d<float> newBasePoint = pos;

	// only update if we are not already very close
	// and if so only move very close to intersection, not to the
	// exact point
	if (colData.nearestDistance >= veryCloseDistance)
	{
		ox::core::CVector3d<float> v = vel;
		{ v.normalize(); v *= ((float)(colData.nearestDistance - veryCloseDistance)); }
		newBasePoint = colData.basePoint + v;

		v.normalize();
		colData.intersectionPoint -= (v * veryCloseDistance);
    }

	// calculate sliding plane

	ox::core::CVector3d<float> slidePlaneOrigin = colData.intersectionPoint;
	ox::core::CVector3d<float> slidePlaneNormal = newBasePoint - colData.intersectionPoint;
	slidePlaneNormal.normalize();
	ox::core::CPlane3d<float> slidingPlane(slidePlaneOrigin, slidePlaneNormal);

	ox::core::CVector3d<float> newDestinationPoint =
		destinationPoint -
		(slidePlaneNormal * slidingPlane.getDistanceTo(destinationPoint));

	// generate slide vector

	ox::core::CVector3d<float> newVelocityVector = newDestinationPoint -
		colData.intersectionPoint;

	if (newVelocityVector.getLength() < veryCloseDistance)
		return newBasePoint;

	return collideWithWorld(recursionDepth+1, colData,
		newBasePoint, newVelocityVector);
}


//! Returns a 3d ray which would go through the 2d screen coodinates.
ox::core::CLine3d<float> CSceneCollisionManager::getRayFromScreenCoordinates(
	ox::core::CPosition2d<int> pos, ICameraSceneNode* camera)
{
	ox::core::CLine3d<float> ln;

	if (!SceneManager)
		return ln;

	if (!camera)
		camera = SceneManager->getActiveCamera();

	if (!camera)
		return ln;

	const ox::scene::SViewFrustrum* f = camera->getViewFrustrum();

	ox::core::CVector3d<float> farLeftUp = f->getFarLeftUp();
	ox::core::CVector3d<float> lefttoright = f->getFarRightUp() - farLeftUp;
	ox::core::CVector3d<float> uptodown = f->getFarLeftDown() - farLeftUp;

	ox::core::CDimension2d<int> screenSize = Driver->getScreenSize();

	float dx = pos.X / (float)screenSize.Width;
	float dy = pos.Y / (float)screenSize.Height;

	ln.start = f->cameraPosition;
	ln.end = farLeftUp + (lefttoright * dx) + (uptodown * dy);

	return ln;
}



//! Calculates 2d screen position from a 3d position.
ox::core::CPosition2d<int> CSceneCollisionManager::getScreenCoordinatesFrom3DPosition(
	ox::core::CVector3d<float> pos3d, ICameraSceneNode* camera)
{
	ox::core::CPosition2d<int> pos2d(-1000,-1000);

	if (!SceneManager || !Driver)
		return pos2d;

	if (!camera)
		camera = SceneManager->getActiveCamera();

	if (!camera)
		return pos2d;

	ox::core::CDimension2d<int> dim = Driver->getScreenSize();
	dim.Width /= 2;
	dim.Height /= 2;

	float transformedPos[4];

	ox::core::CMatrix4 trans = camera->getProjectionMatrix();
	trans *= camera->getViewMatrix();

	transformedPos[0] = pos3d.X;
	transformedPos[1] = pos3d.Y;
	transformedPos[2] = pos3d.Z;
	transformedPos[3] = 1.0f;

	trans.multiplyWith1x4Matrix(transformedPos);

	if (transformedPos[3] < 0)
		return ox::core::CPosition2d<int>(-10000,-10000);

	float zDiv = transformedPos[3] == 0.0f ? 1.0f :
		(1.0f / transformedPos[3]);

	pos2d.X = (int)(dim.Width * transformedPos[0] * zDiv) + dim.Width;
	pos2d.Y = ((int)(dim.Height - (dim.Height * (transformedPos[1] * zDiv))));

	return pos2d;
}


} // end namespace scene
} // end namespace daisy
