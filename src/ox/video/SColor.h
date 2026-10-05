// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/SColor.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::video namespace; not the original source. Partial: only the
// constructors, component getters and alpha setter are recovered.

#ifndef OX_VIDEO_SCOLOR_H
#define OX_VIDEO_SCOLOR_H

namespace ox {
namespace video {

//! A 32 bit A8R8G8B8 color.
class SColor
{
public:
    //! Does nothing: the color value is not initialized.
    SColor() {}

    SColor(int a, int r, int g, int b)
        : color(((a & 0xff) << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff)) {}

    SColor(int clr)
        : color(clr) {}

    //! Returns the alpha component, from 0 (transparent) to 255.
    int getAlpha() const { return (color >> 24) & 0xff; }
    int getRed() const { return (color >> 16) & 0xff; }
    int getGreen() const { return (color >> 8) & 0xff; }
    int getBlue() const { return color & 0xff; }

    //! Changes transparency while keeping the RGB components.
    void setAlpha(int alpha)
    {
        color = ((alpha & 0xff) << 24) | (((color >> 16) & 0xff) << 16) |
            (((color >> 8) & 0xff) << 8) | (color & 0xff);
    }

    //! Sets the red component, from 0 to 255.
    void setRed(int r)
    {
        color = (((color >> 24) & 0xff) << 24) | ((r & 0xff) << 16) | (((color >> 8) & 0xff) << 8) |
            (color & 0xff);
    }

    //! Sets the green component, from 0 to 255.
    void setGreen(int g)
    {
        color = (((color >> 24) & 0xff) << 24) | (((color >> 16) & 0xff) << 16) | ((g & 0xff) << 8) |
            (color & 0xff);
    }

    //! Sets the blue component, from 0 to 255.
    void setBlue(int b)
    {
        color = (((color >> 24) & 0xff) << 24) | (((color >> 16) & 0xff) << 16) |
            (((color >> 8) & 0xff) << 8) | (b & 0xff);
    }

    //! Signed like Irrlicht's s32: CParticleState::update extracts components with arithmetic shifts.
    int color;
};

} // end namespace video
} // end namespace ox

#endif
