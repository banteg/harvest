// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only the corners are recovered.

#ifndef OX_CORE_CAFFINERECT_H
#define OX_CORE_CAFFINERECT_H

#include "CPosition2d.h"

namespace ox {
namespace core {

//! A quad given by its four corners, so it can be rotated, sheared or mirrored.
template <class T>
class CAffineRect
{
public:
    CPosition2d<T> UpperLeftCorner;
    CPosition2d<T> UpperRightCorner;
    CPosition2d<T> LowerLeftCorner;
    CPosition2d<T> LowerRightCorner;
};

} // end namespace core
} // end namespace ox

#endif
