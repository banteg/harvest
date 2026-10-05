// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIPopupMenu.

#ifndef OX_GUI_IGUIPOPUPMENU_H
#define OX_GUI_IGUIPOPUPMENU_H

#include "IGUIElement.h"

namespace ox {
namespace gui {

//! A frame of text options that closes when an option is chosen or the mouse is pressed outside.
class IGUIPopupMenu : public IGUIElement
{
public:
    IGUIPopupMenu(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
    }

    //! Adds an option; its id is the id of the chosen element in EGET_POPUP_MENU_OPTION_CHOSEN.
    virtual void addMenuOption(const wchar_t* text, int id) = 0;
    //! Sets the element that receives the menu's events instead of the parent.
    virtual void setSelectionParent(IGUIElement* parent) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
