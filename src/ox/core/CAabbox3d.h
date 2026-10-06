// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/aabbox3d.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::core namespace; not the original source. Partial: only what
// recovered units use is declared.

#ifndef OX_CORE_CAABBOX3D_H
#define OX_CORE_CAABBOX3D_H

#include "CVector3d.h"

namespace ox {
namespace core {

//! Axis aligned bounding box in 3d space.
template <class T>
class CAabbox3d
{
public:
    CAabbox3d() : MinEdge(-1, -1, -1), MaxEdge(1, 1, 1) {}

    //! Writes the eight corners of the box to edges.
    void getEdges(CVector3d<T>* edges) const
    {
        CVector3d<T> middle = (MinEdge + MaxEdge) / 2;
        CVector3d<T> diag = middle - MaxEdge;

        edges[0].set(middle.X + diag.X, middle.Y + diag.Y, middle.Z + diag.Z);
        edges[1].set(middle.X + diag.X, middle.Y - diag.Y, middle.Z + diag.Z);
        edges[2].set(middle.X + diag.X, middle.Y + diag.Y, middle.Z - diag.Z);
        edges[3].set(middle.X + diag.X, middle.Y - diag.Y, middle.Z - diag.Z);
        edges[4].set(middle.X - diag.X, middle.Y + diag.Y, middle.Z + diag.Z);
        edges[5].set(middle.X - diag.X, middle.Y - diag.Y, middle.Z + diag.Z);
        edges[6].set(middle.X - diag.X, middle.Y + diag.Y, middle.Z - diag.Z);
        edges[7].set(middle.X - diag.X, middle.Y - diag.Y, middle.Z - diag.Z);
    }

    CVector3d<T> MinEdge;
    CVector3d<T> MaxEdge;
};

} // end namespace core
} // end namespace ox

#endif
