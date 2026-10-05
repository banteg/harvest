// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIScrollBar.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUISCROLLBAR_H
#define DAISY_GUI_CGUISCROLLBAR_H

#include "ox/gui/IGUIScrollBar.h"
#include "ox/gui/IGUIButton.h"
#include "ox/video/ISpriteAnimationState.h"

namespace daisy {
namespace gui {

class CGUIListBox;

//! A scroll bar with two buttons, drawn with sprite animations when the skin has a sprite package.
class CGUIScrollBar : public ox::gui::IGUIScrollBar
{
public:
    //! constructor
    CGUIScrollBar(bool horizontal, ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle, bool noclip = false);

    //! destructor
    ~CGUIScrollBar();

    //! Moves the down button along and resizes the background.
    virtual void setRelativePosition(const ox::core::CRect<int>& position);

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! draws the element and its children
    virtual void draw();

    //! sets the position of the scrollbar
    virtual void setPos(int pos);

    //! sets the maximum value of the scrollbar. must be > 0
    virtual void setMax(int max);

    //! sets the steps of the buttons and of clicks beside the thumb
    virtual void setStepSizes(int smallStep, int largeStep);

    virtual int getMax();

    virtual bool isDragging();

    //! gets the current position of the scrollbar
    virtual int getPos();

    //! Uses the animations "<name>Background", "<name>ThumbNormal" ... of the package.
    virtual void setAnimations(ox::video::ISpritePackage* package, const char* name);

    void setListBoxParent(CGUIListBox* listBox);

private:
    //! The animations, and the frame rectangles that are read from them.
    enum EScrollBarAnimation
    {
        ESBA_BACKGROUND = 0,
        ESBA_THUMB_NORMAL,
        ESBA_THUMB_HIGHLIGHTED,
        ESBA_THUMB_PRESSED,
        ESBA_THUMB_DISABLED,
        ESBA_COUNT
    };

    //! Fits the background between the buttons.
    void updateBackgroundSize();

    //! Shows the normal (0), highlighted (1) or pressed (2) thumb.
    void setNewThumbState(int state);

    void setPosFromMousePos(int x, int y);

    ox::gui::IGUIButton* UpButton;
    ox::gui::IGUIButton* DownButton;
    ox::video::ISpriteAnimationState* Animations[ESBA_COUNT];
    ox::video::ISpriteAnimationState* CurrentThumb;
    //! The background rectangle relative to the bar, then the thumb animations' frame sizes.
    ox::core::CRect<int> AnimationRects[ESBA_COUNT];
    //! The rectangle the bar was created with; setAnimations restores it.
    ox::core::CRect<int> OriginalRect;
    bool Dragging;
    bool Horizontal;
    bool NoClip;
    int Pos;
    int DrawPos;
    int DrawHeight;
    int Max;
    int SmallStep;
    int LargeStep;
    CGUIListBox* ListBoxParent;
};

} // end namespace gui
} // end namespace daisy

#endif
