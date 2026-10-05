// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/SLight.h and SColor.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::video namespace; not the original source. The Linux amd64 offsets of
// DiffuseColor (0x10) and Radius (0x3c) are verified.

#ifndef OX_VIDEO_SLIGHT_H
#define OX_VIDEO_SLIGHT_H

#include "../core/CVector3d.h"
#include "SColor.h"

namespace ox {
namespace video {

//! A color with floating point components from 0 to 1.
class SColorf
{
public:
    SColorf()
        : r(0.0f), g(0.0f), b(0.0f), a(0.0f) {}

    SColorf(float r, float g, float b, float a = 1.0f)
        : r(r), g(g), b(b), a(a) {}

    SColorf(SColor c)
    {
        const float inv = 1.0f / 255.0f;
        r = c.getRed() * inv;
        g = c.getGreen() * inv;
        b = c.getBlue() * inv;
        a = c.getAlpha() * inv;
    }

    float r;
    float g;
    float b;
    float a;
};

//! A dynamic light.
struct SLight
{
    SColorf AmbientColor;
    SColorf DiffuseColor;
    SColorf SpecularColor;
    core::CVector3d<float> Position;
    float Radius;
    bool CastShadows;
};

} // end namespace video
} // end namespace ox

#endif
