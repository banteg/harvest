// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/S3DVertex.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::video namespace; not the original source. Partial: only the standard
// vertex is recovered.

#ifndef OX_VIDEO_S3DVERTEX_H
#define OX_VIDEO_S3DVERTEX_H

#include "../core/CVector2d.h"
#include "../core/CVector3d.h"
#include "SColor.h"

namespace ox {
namespace video {

//! The vertex types of mesh buffers, as in Irrlicht 0.7.
enum E_VERTEX_TYPE
{
    //! S3DVertex
    EVT_STANDARD = 0,
    //! S3DVertex2TCoords
    EVT_2TCOORDS
};

//! Standard vertex used by the video driver.
struct S3DVertex
{
    S3DVertex() {}

    core::CVector3d<float> Pos;
    core::CVector3d<float> Normal;
    SColor Color;
    core::CVector2d<float> TCoords;
};

} // end namespace video
} // end namespace ox

#endif
