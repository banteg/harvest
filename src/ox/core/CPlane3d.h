// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/plane3d.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::core namespace; not the original source.

#ifndef __IRR_PLANE_3D_H_INCLUDED__
#define __IRR_PLANE_3D_H_INCLUDED__

#include "irrMath.h"
#include "ox/core/CVector3d.h"

namespace ox
{
namespace core
{

//! Enumeration for intersection relations of 3d objects
enum EIntersectionRelation3D
{
	ISREL3D_FRONT = 0,
	ISREL3D_BACK,
	ISREL3D_PLANAR,
	ISREL3D_SPANNING,
	ISREL3D_CLIPPED
};

//! Template plane class with some intersection testing methods.
template <class T>
class CPlane3d
{
	public:

		// Constructors

		CPlane3d(): Normal(0,1,0) { recalculateD(CVector3d<T>(0,0,0)); };
		CPlane3d(const CVector3d<T>& MPoint, const CVector3d<T>& Normal) : Normal(Normal) { recalculateD(MPoint); };
		CPlane3d(T px, T py, T pz, T nx, T ny, T nz) : Normal(nx, ny, nz) { recalculateD(CVector3d<T>(px, py, pz)); };
		CPlane3d(const CPlane3d<T>& other) :D(other.D), Normal(other.Normal) {};
		CPlane3d(const CVector3d<T>& point1, const CVector3d<T>& point2, const CVector3d<T>& point3)	{ setPlane(point1, point2, point3);	};

		// operators

		inline bool operator==(const CPlane3d<T>& other) const { return (D==other.D && Normal==other.Normal);};
		inline bool operator!=(const CPlane3d<T>& other) const { return !(D==other.D && Normal==other.Normal);};

		// functions

		void setPlane(const CVector3d<T>& point, const CVector3d<T>& nvector)
		{
			Normal = nvector;
			Normal.normalize();
			recalculateD(point);
		}

		void setPlane(const CVector3d<T>& nvect, T d)
		{
			Normal = nvect;
			D = d;
		}

		void setPlane(const CVector3d<T>& point1, const CVector3d<T>& point2, const CVector3d<T>& point3)
		{
			// creates the plane from 3 memberpoints
			Normal = (point2 - point1).crossProduct(point3 - point1);
			Normal.normalize();

			recalculateD(point1);
		}


		//! Returns an intersection with a 3d line.
		//! \param lineVect: Vector of the line to intersect with.
		//! \param linePoint: Point of the line to intersect with.
		//! \param outIntersection: Place to store the intersection point, if there is one.
		//! \return Returns true if there was an intersection, false if there was not.
		bool getIntersectionWithLine(const CVector3d<T>& linePoint, const CVector3d<T>& lineVect,
					CVector3d<T>& outIntersection) const
		{
			T t2 = Normal.dotProduct(lineVect);

			if (t2 == 0)
				return false;

			T t =- (Normal.dotProduct(linePoint) + D) / t2;
			outIntersection = linePoint + (lineVect * t);
			return true;
		}

		//! Returns an intersection with a 3d line, limited between two 3d points.
		//! \param linePoint1: Point 1 of the line.
		//! \param linePoint2: Point 2 of the line.
		//! \param outIntersection: Place to store the intersection point, if there is one.
		//! \return Returns true if there was an intersection, false if there was not.
		bool getIntersectionWithLimitedLine(const CVector3d<T>& linePoint1,
					const CVector3d<T>& linePoint2, CVector3d<T>& outIntersection) const
		{
			return (	getIntersectionWithLine(linePoint1, linePoint2 - linePoint1, outIntersection) &&
						outIntersection.isBetweenPoints(linePoint1, linePoint2));
		}

		//! Classifies the relation of a point to this plane.
		//! \param point: Point to classify its relation.
		//! \return Returns ISREL3D_FRONT if the point is in front of the plane,
		//! ISREL3D_BACK if the point is behind of the plane, and
		//! ISREL3D_PLANAR if the point is within the plane.
		EIntersectionRelation3D classifyPointRelation(const CVector3d<T>& point) const
		{
			float d = Normal.dotProduct(point) + D;

			if (d < -ROUNDING_ERROR)
				return ISREL3D_FRONT;

			if (d > ROUNDING_ERROR)
				return ISREL3D_BACK;

			return ISREL3D_PLANAR;
		}

		//! Recalculates the distance from origin by applying
		//! a new member point to the plane.
		void recalculateD(const CVector3d<T>& MPoint)
		{
			D = - MPoint.dotProduct(Normal);
		}

		//!\return Returns a member point of the plane.
		CVector3d<T> getMemberPoint() const
		{
			return Normal * -D;
		}

		//! Tests if there is a intersection between this plane and another
		//! \return Returns true if there is a intersection.
		bool existsInterSection(const CPlane3d<T>& other) const
		{
			CVector3d<T> cross = other.Normal.crossProduct(Normal);
			return cross.getLength() > 1e-08f;
		}

		//! Intersects this plane with another.
		//! \return Returns true if there is a intersection, false if not.
		bool getIntersectionWithPlane(const CPlane3d<T>& other, CVector3d<T>& outLinePoint,
				CVector3d<T>& outLineVect) const
		{
			double fn00 = Normal.getLength();
			double fn01 = Normal.dotProduct(other.Normal);
			double fn11 = other.Normal.getLength();
			double det = fn00*fn11 - fn01*fn01;

			if (fabs(det) < 1e-08f)
				return false;

            det = 1.0 / det;
			double fc0 = (fn11*-D + fn01*other.D) * det;
			double fc1 = (fn00*-other.D + fn01*D) * det;

			outLineVect = Normal.crossProduct(other.Normal);
			outLinePoint = Normal*(T)fc0 + other.Normal*(T)fc1;
			return true;
		}

		//! Returns the intersection point with two other planes if there is one.
		bool getIntersectionWithPlanes(const CPlane3d<T>& o1,
				const CPlane3d<T>& o2, CVector3d<T>& outPoint) const
		{
			CVector3d<T> linePoint, lineVect;
			if (getIntersectionWithPlane(o1, linePoint, lineVect))
				return o2.getIntersectionWithLine(linePoint, lineVect, outPoint);

			return false;
		}

		//! Returns if the plane is front of backfacing. Note that this only
		//! works if the normal is Normalized.
		//! \param lookDirection: Look direction.
		//! \return Returns true if the plane is front facing, which mean it would
		//! be visible, and false if it is backfacing.
		bool isFrontFacting(const CVector3d<T>& lookDirection) const
		{
			return Normal.dotProduct(lookDirection) <= 0.0f;
		}

		//! Returns the distance to a point.  Note that this only
		//! works if the normal is Normalized.
		T getDistanceTo(const CVector3d<T>& point) const
		{
			return point.dotProduct(Normal) + D;
		}

		// member variables

		T D;					// distance from origin
		CVector3d<T> Normal;		// normal vector
};


//! Typedef for a float 3d plane.
//! Typedef for an integer 3d plane.
typedef CPlane3d<int> plane3di;

} // end namespace core
} // end namespace ox

#endif
