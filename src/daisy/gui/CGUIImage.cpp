// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIImage.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye adds sprite animations and an override color.

#include "CGUIImage.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUISkin.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/ITexture.h"
#include "ox/video/IVideoDriver.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

//! constructor
CGUIImage::CGUIImage(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
    ox::core::CRect<int> rectangle)
    : IGUIImage(environment, parent, id, rectangle), Texture(0), Animation(0), OverrideColor(0xffffffff)
{
}

//! destructor
CGUIImage::~CGUIImage()
{
    if (Texture)
        Texture->drop();

    if (Animation)
        Animation->remove();
}

//! sets an image
void CGUIImage::setImage(ox::video::ITexture* image)
{
    if (Texture)
        Texture->drop();

    Texture = image;

    if (Texture)
        Texture->grab();

    if (Animation)
    {
        Animation->remove();
        Animation = 0;
    }
}

//! shows the named animation of a sprite package and resizes the element to its first frame
void CGUIImage::setAnimation(const char* animation, ox::video::ISpritePackage* package)
{
    if (Texture)
    {
        Texture->drop();
        Texture = 0;
    }

    if (Animation)
    {
        Animation->remove();
        Animation = 0;
    }

    if (package)
    {
        Animation = package->addNewAnimationState(ox::core::CString<char>(animation));
        if (Animation)
            setRelativePosition(ox::core::CRect<int>(RelativeRect.UpperLeftCorner, Animation->getFrameSize(0)));
    }
}

//! sets the color the animation is drawn with
void CGUIImage::setOverrideColor(ox::video::SColor color)
{
    OverrideColor = color;
}

//! draws the element and its children
void CGUIImage::draw()
{
    if (!IsVisible)
        return;

    ox::gui::IGUISkin* skin = Environment->getSkin();
    ox::video::IVideoDriver* driver = Environment->getVideoDriver();

    ox::core::CRect<int> rect = AbsoluteRect;

    if (Texture)
        driver->draw2DImage(Texture, rect.UpperLeftCorner, ox::core::CRect<int>(0, 0, rect.getWidth(), rect.getHeight()),
            &AbsoluteClippingRect, ox::video::SColor(0xffffffff), false);
    else if (Animation)
    {
        ox::core::CPosition2d<int> offset = Animation->getFrameOffset(0);
        Animation->draw(rect.UpperLeftCorner - offset, &AbsoluteClippingRect, OverrideColor);
    }
    else
        driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_DARK_SHADOW), rect, &AbsoluteClippingRect);

    IGUIElement::draw();
}

} // end namespace gui
} // end namespace daisy
