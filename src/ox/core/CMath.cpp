// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CMath.h"
#include <algorithm>
#include <cmath>
// The native unit has an iostream static initializer, as do the other ox units.
#include <iostream> // IWYU pragma: keep

namespace ox {
namespace core {

static const float PI = 3.14159274f;
static const float SQRT_2 = 1.41421354f;

//! True when value lies strictly between a and b.
static inline bool isBetween(float value, float a, float b)
{
    return (a > value && value > b) || (value > a && b > value);
}

float CMath::getEstimateDistance(const CPosition2d<float>& a, const CPosition2d<float>& b)
{
    float dx = a.X - b.X;
    float dy = a.Y - b.Y;
    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;

    if (dx > dy)
        std::swap(dx, dy);
    return dy - dx + dx * SQRT_2;
}

float CMath::getExactDistance(const CPosition2d<float>& a, const CPosition2d<float>& b)
{
    float dx = a.X - b.X;
    float dy = a.Y - b.Y;
    return sqrtf(dx * dx + dy * dy);
}

float CMath::getSquaredDistance(const CPosition2d<float>& a, const CPosition2d<float>& b)
{
    float dx = a.X - b.X;
    float dy = a.Y - b.Y;
    return dx * dx + dy * dy;
}

float CMath::getAngleIY(const CPosition2d<float>& a, const CPosition2d<float>& b)
{
    float dx = b.X - a.X;
    float dy = b.Y - a.Y;
    if (dx == 0)
        return dy < 0 ? PI * 1.5f : PI * 0.5f;

    float angle = atanf(dy / dx);
    if (dx < 0)
        angle += PI;
    return angle;
}

bool CMath::lineIntersects(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4,
    float& x, float& y)
{
    if (x2 == x1)
    {
        if (x4 == x3)
        {
            // both lines are vertical
            if (x3 != x1)
                return false;
            x = x1;
            if (isBetween(y3, y1, y2))
            {
                y = y3;
                return true;
            }
            if (isBetween(y4, y1, y2))
            {
                y = y4;
                return true;
            }
            return false;
        }

        if ((x3 > x1 && x4 > x1) || (x1 > x3 && x1 > x4))
            return false;
        float iy = (x1 - x3) * ((y4 - y3) / (x4 - x3)) + y3;
        if ((iy > y1 && iy > y2) || (y1 > iy && y2 > iy))
            return false;
        x = x1;
        y = iy;
        return true;
    }

    float dx34 = x4 - x3;
    if (dx34 == 0)
    {
        if ((x1 > x3 && x2 > x3) || (x3 > x1 && x3 > x2))
            return false;
        float iy = (x3 - x1) * ((y2 - y1) / (x2 - x1)) + y1;
        if ((iy > y3 && iy > y4) || (y3 > iy && y4 > iy))
            return false;
        x = x3;
        y = iy;
        return true;
    }

    if (y1 == y2)
    {
        if (y3 == y4)
        {
            // both lines are horizontal
            if (y3 != y1)
                return false;
            y = y1;
            if (isBetween(x3, x1, x2))
            {
                x = x3;
                return true;
            }
            if (isBetween(x4, x1, x2))
            {
                x = x4;
                return true;
            }
            return false;
        }

        if ((y3 > y1 && y4 > y1) || (y1 > y3 && y1 > y4))
            return false;
        float ix = (y1 - y3) * (dx34 / (y4 - y3)) + x3;
        if ((ix > x1 && ix > x2) || (x1 > ix && x2 > ix))
            return false;
        x = ix;
        y = y1;
        return true;
    }

    if (y3 == y4)
    {
        if ((y1 > y3 && y2 > y3) || (y3 > y1 && y3 > y2))
            return false;
        float ix = (y3 - y1) * ((x2 - x1) / (y2 - y1)) + x1;
        if ((ix > x3 && ix > x4) || (x3 > ix && x4 > ix))
            return false;
        x = ix;
        y = y3;
        return true;
    }

    float m1 = (y2 - y1) / (x2 - x1);
    float m2 = (y4 - y3) / dx34;
    if (m1 == m2)
        return false;

    float b1 = y1 - m1 * x1;
    float b2 = y3 - m2 * x3;
    float ix = -(b1 - b2) / (m1 - m2);
    if ((x1 - ix) * (ix - x2) >= 0 && (x3 - ix) * (ix - x4) >= 0)
    {
        float iy = m1 * ix + b1;
        if ((y1 - iy) * (iy - y2) >= 0 && (y3 - iy) * (iy - y4) >= 0)
        {
            x = ix;
            y = iy;
            return true;
        }
    }
    return false;
}

bool CMath::pointInFrontOfLine(float x, float y, float x1, float y1, float x2, float y2)
{
    return (x - x1) * (y2 - y1) - (y - y1) * (x2 - x1) >= 0;
}

bool CMath::clockWiseClosestIY(float a, float b)
{
    float difference = b - a;
    if (difference > 0)
        return PI > difference;
    return -PI > difference;
}

void CMath::hermiteInterpolation(float t, CVector3d<float>& result, const CVector3d<float>* points)
{
    float h00 = 2 * t * t * t - 3 * t * t + 1;
    float h01 = -2 * t * t * t + 3 * t * t;
    float h10 = t * t * t - 2 * t * t + t;
    float h11 = t * t * t - t * t;

    result = points[1] * h00 + points[2] * h01 + (points[2] - points[0]) * 0.5f * h10
        + (points[3] - points[1]) * 0.5f * h11;
}

} // end namespace core
} // end namespace ox
