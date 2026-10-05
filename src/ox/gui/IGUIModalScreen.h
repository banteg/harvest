// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIModalScreen.

#ifndef OX_GUI_IGUIMODALSCREEN_H
#define OX_GUI_IGUIMODALSCREEN_H

#include "IGUILayout.h"

namespace ox {
namespace gui {

//! A screen-filling layout that blocks the events of the elements behind it.
class IGUIModalScreen : public IGUILayout
{
public:
    IGUIModalScreen(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUILayout(environment, parent, id, rectangle)
    {
        Type = EGUIET_MODAL_SCREEN;
    }

    //! The event of the last EGET_MODAL_SCREEN_BLOCKED notification.
    virtual event::SEvent getLastBlockedEvent() = 0;
};

} // end namespace gui
} // end namespace ox

#endif
