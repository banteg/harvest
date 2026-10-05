// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef OX_CORE_CMATH_H
#define OX_CORE_CMATH_H

#include "CPosition2d.h"

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
};

} // end namespace core
} // end namespace ox

#endif
