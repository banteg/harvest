// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIModalScreen.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye tints the screen instead of flashing the children's frames and reports blocked input to the
// parent with EGET_MODAL_SCREEN_BLOCKED.

#include "CGUIModalScreen.h"
#include "ox/gui/IGUIElementInline.h"
#include "ox/gui/IGUILayoutInline.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUISkin.h"
#include "ox/video/IVideoDriver.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

//! constructor
CGUIModalScreen::CGUIModalScreen(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id)
    : IGUIModalScreen(environment, parent, id, parent->getAbsolutePosition())
{
}

//! destructor
CGUIModalScreen::~CGUIModalScreen()
{
}

//! called if an event happened.
bool CGUIModalScreen::OnEvent(const ox::event::SEvent& event)
{
    if (event.EventType == ox::event::EET_MOUSE_INPUT_EVENT || event.EventType == ox::event::EET_KEY_INPUT_EVENT)
    {
        LastBlockedEvent = event;

        if (Parent)
        {
            ox::event::SEvent blocked;
            blocked.EventType = ox::event::EET_GUI_EVENT;
            blocked.GUIEvent.Caller = this;
            blocked.GUIEvent.EventType = ox::gui::EGET_MODAL_SCREEN_BLOCKED;
            IGUIElement::OnEvent(blocked);
        }
        return true;
    }

    return IGUIElement::OnEvent(event);
}

//! draws the element and its children
void CGUIModalScreen::draw()
{
    if (!IsVisible)
        return;

    if (Parent)
    {
        ox::core::CRect<int> rect = Parent->getAbsolutePosition();
        ox::video::SColor color = Environment->getSkin()->getColor(ox::gui::EGDC_MODAL_SCREEN);
        Environment->getVideoDriver()->draw2DRectangle(color, rect, 0);
    }

    IGUIElement::draw();
}

//! Removes a child.
void CGUIModalScreen::removeChild(ox::gui::IGUIElement* child)
{
    IGUIElement::removeChild(child);

    if (Children.empty())
        remove();
}

void CGUIModalScreen::updateAbsolutePosition()
{
    ox::core::CRect<int> parentRect(0, 0, 0, 0);

    if (Parent)
    {
        parentRect = Parent->getAbsolutePosition();
        RelativeRect.UpperLeftCorner.X = 0;
        RelativeRect.UpperLeftCorner.Y = 0;
        RelativeRect.LowerRightCorner.X = parentRect.getWidth();
        RelativeRect.LowerRightCorner.Y = parentRect.getHeight();
    }

    IGUIElement::updateAbsolutePosition();
}

ox::event::SEvent CGUIModalScreen::getLastBlockedEvent()
{
    return LastBlockedEvent;
}

} // end namespace gui
} // end namespace daisy
