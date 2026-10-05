// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIModalScreen.

#ifndef OX_GUI_IGUIMODALSCREEN_H
#define OX_GUI_IGUIMODALSCREEN_H

#include "IGUILayout.h"

namespace ox {
namespace gui {

//! A layout covering the screen that blocks the input to the elements behind it.
class IGUIModalScreen : public IGUILayout
{
public:
    //! The last event the screen kept from the elements behind it.
    virtual event::SEvent getLastBlockedEvent() = 0;
};

} // end namespace gui
} // end namespace ox

#endif
