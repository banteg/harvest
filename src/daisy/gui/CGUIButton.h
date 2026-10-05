// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIButton.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye draws the button states with sprite animations and adds CGUITextButton, which the Mac
// build compiles in the same object.

#ifndef DAISY_GUI_CGUIBUTTON_H
#define DAISY_GUI_CGUIBUTTON_H

#include "ox/gui/IGUIButton.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CString.h"

namespace ox {
namespace video { class ISpriteAnimationState; }
} // end namespace ox

namespace daisy {
namespace gui {

//! A button drawn with one sprite animation per state, or with skin colors without a sprite package.
class CGUIButton : public ox::gui::IGUIButton
{
public:
    //! constructor; a text starting with character 0x103 makes a window close button
    CGUIButton(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle, bool noclip = false, const wchar_t* text = 0);

    //! destructor
    virtual ~CGUIButton();

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! draws the element and its children
    virtual void draw();

    //! sets another skin independent font. if this is set to zero, the button uses the font of the skin.
    virtual void setOverrideFont(ox::gui::IGUIFont* font);

    //! sets a text color that replaces the skin's button text colors
    virtual void setOverrideColor(const ox::video::SColor& color);

    //! starts the animations "<name>Normal", "<name>Highlighted", "<name>Pressed" and "<name>Disabled";
    //! resize makes the button as large as the normal animation
    virtual void setAnimations(ox::video::ISpritePackage* package, const char* name, bool resize);

    //! Sets if the button should behave like a push button.
    virtual void setIsPushButton(bool isPushButton);

    //! Sets the pressed state of the button
    virtual void setPressed(bool pressed);

    //! Returns if the button is currently pressed
    virtual bool isPressed();

    //! sends a click to the parent as if the button had been clicked
    virtual void fakeButtonClick();

private:
    //! switches to a state and restarts its animation
    void setNewState(ox::gui::EButtonStates state);

    ox::gui::EButtonStates State;
    bool NoClip;
    bool IsPushButton;
    ox::gui::IGUIFont* OverrideFont;
    ox::video::SColor OverrideColor;
    bool OverrideColorEnabled;
    ox::video::ISpriteAnimationState* States[ox::gui::EBS_COUNT];
};

//! A text that acts as a button, with a sprite on both sides while the mouse is over it and an
//! optional sprite above it.
class CGUITextButton : public ox::gui::IGUITextButton
{
public:
    //! constructor; the element is sized to the text and the sprites
    CGUITextButton(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, const wchar_t* text,
        int id, ox::gui::IGUIFont* font, ox::video::SColor color, const char* sideSprite, const char* topSprite);

    //! destructor
    virtual ~CGUITextButton();

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! draws the element and its children
    virtual void draw();

    virtual void setTextColor(ox::video::SColor color);

    //! does nothing
    void setAnimations(ox::video::ISpritePackage* package, const char* name, bool resize);

private:
    enum ESprite
    {
        ES_SIDE = 0,
        ES_TOP,
        ES_UNUSED,
        ES_COUNT
    };

    ox::gui::IGUIFont* Font;
    ox::video::ISpriteAnimationState* Sprites[ES_COUNT];
    ox::core::CString<wchar_t> ButtonText;
    ox::core::CDimension2d<int> TextSize;
    ox::video::SColor Color;
};

} // end namespace gui
} // end namespace daisy

#endif
