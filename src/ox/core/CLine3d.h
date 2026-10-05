// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/line3d.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::core namespace; not the original source. Partial: only what recovered
// units use.

#ifndef OX_CORE_CLINE3D_H
#define OX_CORE_CLINE3D_H

#include "CVector3d.h"

namespace ox {
namespace core {

//! 3D line between two points.
template <class T>
class CLine3d
{
public:
    CLine3d()
        : start(0, 0, 0), end(1, 1, 1) {}

    CLine3d(const CVector3d<T>& start, const CVector3d<T>& end)
        : start(start), end(end) {}

    //! Returns the point of the line closest to a point.
    CVector3d<T> getClosestPoint(const CVector3d<T>& point) const
    {
        CVector3d<T> c = point - start;
        CVector3d<T> v = end - start;
        T d = (T)v.getLength();
        v /= d;
        T t = v.dotProduct(c);

        if (t < (T)0.0)
            return start;
        if (t > d)
            return end;

        v *= t;
        return start + v;
    }

    CVector3d<T> start;
    CVector3d<T> end;
};

} // end namespace core
} // end namespace ox

#endif
