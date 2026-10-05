// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/rect.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::core namespace; not the original source. Partial: the geometry
// helpers are not recovered yet.

#ifndef OX_CORE_CRECT_H
#define OX_CORE_CRECT_H

#include "CDimension2d.h"
#include "CPosition2d.h"

namespace ox {
namespace core {

//! Rectangle template with an upper left and a lower right corner.
template <class T>
class CRect
{
public:
    CRect()
        : UpperLeftCorner(0, 0), LowerRightCorner(0, 0) {}

    CRect(T x, T y, T x2, T y2)
        : UpperLeftCorner(x, y), LowerRightCorner(x2, y2) {}

    CRect(const CPosition2d<T>& upperLeft, const CPosition2d<T>& lowerRight)
        : UpperLeftCorner(upperLeft), LowerRightCorner(lowerRight) {}

    CRect(const CPosition2d<T>& position, const CDimension2d<T>& size)
        : UpperLeftCorner(position), LowerRightCorner(position.X + size.Width, position.Y + size.Height) {}

    CRect(const CRect<T>& other)
        : UpperLeftCorner(other.UpperLeftCorner), LowerRightCorner(other.LowerRightCorner) {}

    const CRect<T>& operator=(const CRect<T>& other)
    {
        UpperLeftCorner = other.UpperLeftCorner;
        LowerRightCorner = other.LowerRightCorner;
        return *this;
    }

    T getWidth() const
    {
        return LowerRightCorner.X - UpperLeftCorner.X;
    }

    T getHeight() const
    {
        return LowerRightCorner.Y - UpperLeftCorner.Y;
    }

    bool isPointInside(const CPosition2d<T>& position) const
    {
        return position.X >= UpperLeftCorner.X && position.Y >= UpperLeftCorner.Y &&
            position.X < LowerRightCorner.X && position.Y < LowerRightCorner.Y;
    }

    bool isRectCollided(const CRect<T>& other) const
    {
        return LowerRightCorner.Y > other.UpperLeftCorner.Y &&
            UpperLeftCorner.Y < other.LowerRightCorner.Y &&
            LowerRightCorner.X > other.UpperLeftCorner.X &&
            UpperLeftCorner.X < other.LowerRightCorner.X;
    }

    CPosition2d<T> UpperLeftCorner;
    CPosition2d<T> LowerRightCorner;
};

} // end namespace core
} // end namespace ox

#endif
