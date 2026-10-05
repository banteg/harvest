// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIEditBox.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUIEDITBOX_H
#define DAISY_GUI_CGUIEDITBOX_H

#include "ox/gui/IGUIEditBox.h"
#include "ox/IOSOperator.h"

namespace ox {
namespace video { class ISpriteAnimationState; }
}

namespace daisy {
namespace gui {

//! A single line text input, drawn with the skin's 3d border or with sprite animations.
class CGUIEditBox : public ox::gui::IGUIEditBox
{
public:
    //! constructor
    CGUIEditBox(const wchar_t* text, bool border, ox::gui::IGUIEnvironment* environment,
        ox::gui::IGUIElement* parent, int id, const ox::core::CRect<int>& rectangle, ox::IOSOperator* op);

    //! destructor
    ~CGUIEditBox();

    //! Moves the right animation along with the right edge.
    virtual void setRelativePosition(const ox::core::CRect<int>& position);

    //! Takes the focus and selects the whole text.
    virtual void setFocus();

    //! Sets another skin independent font.
    virtual void setOverrideFont(ox::gui::IGUIFont* font = 0);

    //! Sets another color for the text.
    virtual void setOverrideColor(ox::video::SColor color);

    //! Sets if the text should use the overide color or the
    //! color in the gui skin.
    virtual void enableOverrideColor(bool enable);

    //! The button that Enter clicks.
    virtual void setAssociatedButton(int id);

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! draws the element and its children
    virtual void draw();

    //! Sets the new caption of this element.
    virtual void setText(const wchar_t* text);

    //! Sets the maximum amount of characters which may be entered in the box.
    //! \param max: Maximum amount of characters. If 0, the character amount is
    //! infinity.
    virtual void setMax(int max);

    //! Returns maximum amount of characters, previously set by setMax();
    virtual int getMax();

    //! Shows the text as asterisks.
    virtual void setHidden(bool hidden);

    //! Draws the box with the animations name + "Background", "Left" and "Right" of a package.
    virtual void setAnimations(ox::video::ISpritePackage* package, const char* name);

protected:
    //! The parts of a box drawn with sprite animations.
    enum EEDITBOX_ANIMATION
    {
        EEBA_BACKGROUND = 0,
        EEBA_LEFT,
        EEBA_RIGHT,
        EEBA_COUNT
    };

    bool processKey(const ox::event::SEvent& event);
    bool processMouse(const ox::event::SEvent& event);
    int getCursorPos(int x);
    //! Fits the background between the left and right animations after a resize.
    void updateAnimationRects();

    bool MouseMarking;
    bool Border;
    bool OverrideColorEnabled;
    int MarkBegin;
    int MarkEnd;

    ox::video::SColor OverrideColor;
    ox::gui::IGUIFont* OverrideFont;
    ox::IOSOperator* Operator;

    //! The background, left and right animations and their rectangles relative to the box.
    ox::video::ISpriteAnimationState* Animations[EEBA_COUNT];
    ox::core::CRect<int> AnimationRects[EEBA_COUNT];

    unsigned int BlinkStartTime;
    int CursorPos;
    int ScrollPos; // scrollpos in characters
    int Max;
    bool Hidden;
    //! The id of the button that Enter clicks, or -1.
    int AssociatedButton;
};

} // end namespace gui
} // end namespace daisy

#endif
