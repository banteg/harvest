// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef OX_CORE_CMATH_H
#define OX_CORE_CMATH_H

#include "CPosition2d.h"
#include "CVector3d.h"

namespace ox {
namespace core {

//! Geometry helpers.
class CMath
{
public:
    static float getEstimateDistance(const CPosition2d<float>& a, const CPosition2d<float>& b);
    static float getAngleIY(const CPosition2d<float>& a, const CPosition2d<float>& b);
    static bool clockWiseClosestIY(float a, float b);
    static float getSquaredDistance(const CPosition2d<float>& a, const CPosition2d<float>& b);
    static float getExactDistance(const CPosition2d<float>& a, const CPosition2d<float>& b);
    //! Intersects the segments (x1, y1)-(x2, y2) and (x3, y3)-(x4, y4); returns false when they do
    //! not cross or are parallel, otherwise the intersection in x and y.
    static bool lineIntersects(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4,
        float& x, float& y);
    //! True when (x, y) is on the line from (x1, y1) to (x2, y2) or on its front side, the side its
    //! cross product with the line direction is positive.
    static bool pointInFrontOfLine(float x, float y, float x1, float y1, float x2, float y2);
    //! The Catmull-Rom spline through points[1] and points[2] at t in [0, 1].
    static void hermiteInterpolation(float t, CVector3d<float>& result, const CVector3d<float>* points);
};

} // end namespace core
} // end namespace ox

#endif
