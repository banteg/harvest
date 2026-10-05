// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUICheckBox.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye draws the box with sprite animations and gives the text its own font and color.

#ifndef DAISY_GUI_CGUICHECKBOX_H
#define DAISY_GUI_CGUICHECKBOX_H

#include "ox/gui/IGUICheckBox.h"

namespace ox {
namespace video { class ISpriteAnimationState; }
} // end namespace ox

namespace daisy {
namespace gui {

//! A check box with a text, drawn with one sprite animation per state.
class CGUICheckBox : public ox::gui::IGUICheckBox
{
public:
    //! constructor
    CGUICheckBox(bool checked, ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle);

    //! destructor
    virtual ~CGUICheckBox();

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! draws the element and its children
    virtual void draw();

    //! set if box is checked
    virtual void setChecked(bool checked);

    //! returns if box is checked
    virtual bool isChecked();

    //! starts the animations "<name>Normal", "<name>Highlighted", "<name>Disabled" and their "Checked"
    //! variants, and makes the element large enough for them
    virtual void setAnimations(ox::video::ISpritePackage* package, const char* name);

    virtual void setTextColor(ox::video::SColor color);

    virtual void setTextFont(ox::gui::IGUIFont* font);

    //! widens the element to fit the box and the text
    virtual void updateWidth();

private:
    //! The animations, in the order of CHECKBOX_ANIMATION_NAMES.
    enum EAnimation
    {
        EA_NORMAL = 0,
        EA_HIGHLIGHTED,
        EA_DISABLED,
        EA_CHECKED_NORMAL,
        EA_CHECKED_HIGHLIGHTED,
        EA_CHECKED_DISABLED,
        EA_COUNT
    };

    bool Pressed;
    bool Checked;
    bool Highlighted;
    ox::video::ISpriteAnimationState* Animations[EA_COUNT];
    ox::video::SColor TextColor;
    ox::gui::IGUIFont* TextFont;
};

} // end namespace gui
} // end namespace daisy

#endif
