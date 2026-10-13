// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/triangle3d.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::core namespace; not the original source. Partial: only what
// recovered units use is declared.

#ifndef OX_CORE_CTRIANGLE3D_H
#define OX_CORE_CTRIANGLE3D_H

#include "CVector3d.h"
#include "ox/core/CLine3d.h"
#include "ox/core/CPlane3d.h"
#include "ox/core/CAabbox3d.h"

namespace ox {
namespace core {

//! 3d triangle template class.
template <class T>
class CTriangle3d
{
public:
    bool operator==(const CTriangle3d<T>& other) const { return other.pointA==pointA && other.pointB==pointB && other.pointC==pointC; }

    void set(const CVector3d<T>& a, const CVector3d<T>& b, const CVector3d<T>& c)
    {
        pointA = a;
        pointB = b;
        pointC = c;
    }

    bool isPointInsideFast(const CVector3d<T>& p) const
    {
        CVector3d<T> f = pointB - pointA;
        CVector3d<T> g = pointC - pointA;

        float a = f.dotProduct(f);
        float b = f.dotProduct(g);
        float c = g.dotProduct(g);

        float ac_bb = (a*c)-(b*b);
        CVector3d<T> vp = p - pointA;

        float d = vp.dotProduct(f);
        float e = vp.dotProduct(g);
        float x = (d*c)-(e*b);
        float y = (e*a)-(d*b);
        float z = x+y-ac_bb;

        return (( ((unsigned int&)z)& ~(((unsigned int&)x)|((unsigned int&)y))) & 0x80000000)!=0;
    }

    CPlane3d<T> getPlane() const
    {
        return CPlane3d<T>(pointA, pointB, pointC);
    }

    bool isTotalInsideBox(const CAabbox3d<float>& box) const
    {
        return (box.isPointInside(pointA) &&
                box.isPointInside(pointB) &&
                box.isPointInside(pointC));
    }

    CVector3d<T> pointA;
    CVector3d<T> pointB;
    CVector3d<T> pointC;
};

} // end namespace core
} // end namespace ox

#endif
