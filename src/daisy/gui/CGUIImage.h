// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIImage.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUIIMAGE_H
#define DAISY_GUI_CGUIIMAGE_H

#include "ox/gui/IGUIImage.h"

namespace ox {
namespace video {
class ISpriteAnimationState;
} // end namespace video
} // end namespace ox

namespace daisy {
namespace gui {

//! Shows a texture or a running sprite animation; without either it draws a dark rectangle.
class CGUIImage : public ox::gui::IGUIImage
{
public:
    //! constructor
    CGUIImage(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle);

    //! destructor
    ~CGUIImage();

    //! sets an image
    virtual void setImage(ox::video::ITexture* image);

    //! shows the named animation of a sprite package and resizes the element to its first frame
    virtual void setAnimation(const char* animation, ox::video::ISpritePackage* package);

    //! sets the color the animation is drawn with
    virtual void setOverrideColor(ox::video::SColor color);

    //! draws the element and its children
    virtual void draw();

private:
    ox::video::ITexture* Texture;
    ox::video::ISpriteAnimationState* Animation;
    ox::video::SColor OverrideColor;
};

} // end namespace gui
} // end namespace daisy

#endif
