// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIWindow.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye draws the window from sprite animations when the skin has them and adds frames, windows
// without title bar buttons.

#ifndef DAISY_GUI_CGUIWINDOW_H
#define DAISY_GUI_CGUIWINDOW_H

#include "ox/gui/IGUIWindow.h"

namespace ox {
namespace video { class ISpriteAnimationState; }
}

namespace daisy {
namespace gui {

//! The parts of a window or frame drawn from sprite animations, in setAnimations order.
enum EWINDOW_PART
{
    EWP_BACKGROUND = 0,
    EWP_TOP_LEFT,
    EWP_TOP_RIGHT,
    EWP_BOTTOM_LEFT,
    EWP_BOTTOM_RIGHT,
    EWP_TOP,
    EWP_LEFT,
    EWP_RIGHT,
    EWP_BOTTOM,
    EWP_COUNT
};

//! The inner edges drawn over the background, in setAnimations order.
enum EWINDOW_INNER_PART
{
    EWIP_TOP = 0,
    EWIP_LEFT,
    EWIP_RIGHT,
    EWIP_BOTTOM,
    EWIP_COUNT
};

class CGUIWindow : public ox::gui::IGUIWindow
{
public:
    //! constructor; a frame has no title bar buttons and no caption.
    CGUIWindow(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle, bool isFrame);

    //! destructor
    virtual ~CGUIWindow();

    virtual void setRelativePosition(const ox::core::CRect<int>& position);

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! draws the element and its children
    virtual void draw();

    //! Returns pointer to the close button
    virtual ox::gui::IGUIButton* getCloseButton();

    //! Returns pointer to the minimize button
    virtual ox::gui::IGUIButton* getMinimizeButton();

    //! Returns pointer to the maximize button
    virtual ox::gui::IGUIButton* getMaximizeButton();

    virtual void setAnimations(ox::video::ISpritePackage* package, const char* animation);

    virtual ox::core::CRect<int> getContentArea();

protected:
    //! Stretches the edge and background rectangles to the window size.
    void updateAnimationRects();

    ox::core::CPosition2d<int> DragStart;
    bool Dragging;
    bool IsFrame;

    ox::gui::IGUIButton* CloseButton;
    ox::gui::IGUIButton* MinButton;
    ox::gui::IGUIButton* RestoreButton;

    ox::video::ISpriteAnimationState* Animations[EWP_COUNT];
    //! Where each part is drawn, relative to the window.
    ox::core::CRect<int> AnimationRects[EWP_COUNT];
    ox::video::ISpriteAnimationState* InnerAnimations[EWIP_COUNT];
    //! The frame sizes of the inner edges.
    ox::core::CRect<int> InnerAnimationRects[EWIP_COUNT];
};

} // end namespace gui
} // end namespace daisy

#endif
