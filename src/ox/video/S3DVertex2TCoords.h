// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/S3DVertex.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::video namespace; not the original source.
// Irrlicht defines this next to S3DVertex; it lives apart so that units which only pass it by
// pointer (IVideoDriver.h forward-declares it) keep their GCC 4.4 code generation.

#ifndef OX_VIDEO_S3DVERTEX2TCOORDS_H
#define OX_VIDEO_S3DVERTEX2TCOORDS_H

#include "S3DVertex.h"

namespace ox {
namespace video {

//! Vertex with a second set of texture coordinates, for lightmaps and two layer materials.
struct S3DVertex2TCoords
{
    S3DVertex2TCoords() {}

    core::CVector3d<float> Pos;
    core::CVector3d<float> Normal;
    SColor Color;
    core::CVector2d<float> TCoords;
    core::CVector2d<float> TCoords2;
};

} // end namespace video
} // end namespace ox

#endif
