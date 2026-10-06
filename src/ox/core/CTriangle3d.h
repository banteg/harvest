// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/triangle3d.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::core namespace; not the original source. Partial: only what
// recovered units use is declared.

#ifndef OX_CORE_CTRIANGLE3D_H
#define OX_CORE_CTRIANGLE3D_H

#include "CVector3d.h"

namespace ox {
namespace core {

//! 3d triangle template class.
template <class T>
class CTriangle3d
{
public:
    CVector3d<T> pointA;
    CVector3d<T> pointB;
    CVector3d<T> pointC;
};

} // end namespace core
} // end namespace ox

#endif
