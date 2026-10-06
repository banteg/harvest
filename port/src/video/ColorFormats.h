// Colour formats and conversions the port shares between its image code and the renderer.
// See docs/port/textures.md ("Colour formats").

#ifndef HARVEST_PORT_VIDEO_COLORFORMATS_H
#define HARVEST_PORT_VIDEO_COLORFORMATS_H

#include "ox/video/IVideoDriver.h"

namespace port {

//! The ECOLOR_FORMAT values the recovered enum leaves out (Irrlicht's numbering).
const ox::video::ECOLOR_FORMAT ECF_R5G6B5 = (ox::video::ECOLOR_FORMAT)1;
const ox::video::ECOLOR_FORMAT ECF_R8G8B8 = (ox::video::ECOLOR_FORMAT)2;

//! An A1R5G5B5 pixel as A8R8G8B8; the low three bits of each channel stay zero.
inline unsigned int A1R5G5B5toA8R8G8B8(unsigned short color)
{
    return ((color & 0x8000) ? 0xff000000u : 0) | ((color & 0x7c00) << 9) | ((color & 0x3e0) << 6) |
        ((color & 0x1f) << 3);
}

} // end namespace port

#endif
