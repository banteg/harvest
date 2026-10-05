// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CGUIClickArea.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

using namespace ox;
using namespace ox::gui;

CClickArea::CClickArea(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
    : IGUIClickArea(environment, parent, id, rectangle)
{
}

CClickArea::~CClickArea()
{
}

//! called if an event happened.
bool CClickArea::OnEvent(const event::SEvent& event)
{
    if (event.EventType == event::EET_MOUSE_INPUT_EVENT)
    {
        event::SEvent e;

        switch (event.MouseInput.Event)
        {
        case event::EMIE_LMOUSE_PRESSED_DOWN:
            e.GUIEvent.EventType = EGET_CLICK_AREA_LEFT_DOWN;
            break;
        case event::EMIE_RMOUSE_PRESSED_DOWN:
            e.GUIEvent.EventType = EGET_CLICK_AREA_RIGHT_DOWN;
            break;
        case event::EMIE_MMOUSE_PRESSED_DOWN:
            e.GUIEvent.EventType = EGET_CLICK_AREA_MIDDLE_DOWN;
            break;
        case event::EMIE_LMOUSE_LEFT_UP:
            e.GUIEvent.EventType = EGET_CLICK_AREA_LEFT_UP;
            break;
        case event::EMIE_RMOUSE_LEFT_UP:
            e.GUIEvent.EventType = EGET_CLICK_AREA_RIGHT_UP;
            break;
        case event::EMIE_MMOUSE_LEFT_UP:
            e.GUIEvent.EventType = EGET_CLICK_AREA_MIDDLE_UP;
            break;
        default:
            return Parent ? Parent->OnEvent(event) : false;
        }

        if (Parent)
        {
            e.EventType = event::EET_GUI_EVENT;
            e.GUIEvent.Caller = this;
            Parent->OnEvent(e);
            return true;
        }
    }

    return Parent ? Parent->OnEvent(event) : false;
}

//! draws the children
void CClickArea::draw()
{
    if (IsVisible)
        IGUIElement::draw();
}

} // end namespace gui
} // end namespace daisy
