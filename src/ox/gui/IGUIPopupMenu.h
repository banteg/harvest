// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the methods CGUIDetachableFrame calls, in the Mac 1.18 vtable order of
// daisy::gui::CGUIPopupMenu after the IGUIElement slots.

#ifndef OX_GUI_IGUIPOPUPMENU_H
#define OX_GUI_IGUIPOPUPMENU_H

#include "IGUIElement.h"

namespace ox {
namespace gui {

//! A popup list of menu options.
class IGUIPopupMenu : public IGUIElement
{
public:
    virtual void addMenuOption(const wchar_t* text, int id) = 0;
    //! Sets the element that receives the selection event.
    virtual void setSelectionParent(IGUIElement* parent) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
