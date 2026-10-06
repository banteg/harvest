// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/S3DVertex.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::video namespace; not the original source.
// Inline body of the full S3DVertex constructor, kept out of S3DVertex.h (Irrlicht defines it in
// the class): extra declarations in that widely included header perturb GCC 4.4 register choices
// in unrelated units such as CVideoNull.

#ifndef OX_VIDEO_S3DVERTEXINLINE_H
#define OX_VIDEO_S3DVERTEXINLINE_H

#include "S3DVertex.h"

namespace ox {
namespace video {

inline S3DVertex::S3DVertex(float x, float y, float z, float nx, float ny, float nz, SColor c, float tu, float tv)
    : Pos(x, y, z), Normal(nx, ny, nz), Color(c), TCoords(tu, tv)
{
}

} // end namespace video
} // end namespace ox

#endif
