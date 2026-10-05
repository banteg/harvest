// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIContextMenu.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. The virtual order follows the
// Mac 1.18 vtable of daisy::gui::CGUIContextMenu.

#ifndef OX_GUI_IGUICONTEXTMENU_H
#define OX_GUI_IGUICONTEXTMENU_H

#include "IGUIElement.h"

namespace ox {
namespace gui {

//! A context menu with items and sub menus.
class IGUIContextMenu : public IGUIElement
{
public:
    IGUIContextMenu(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
    }

    virtual int getItemCount() const = 0;
    //! Adds an item and returns its index; a null text adds a separator.
    virtual int addItem(wchar_t* text, int commandId = -1, bool enabled = true, bool hasSubMenu = false) = 0;
    virtual void addSeparator() = 0;
    virtual const wchar_t* getItemText(int idx) = 0;
    virtual void setItemText(int idx, const wchar_t* text) = 0;
    virtual bool isItemEnabled(int idx) = 0;
    virtual void setItemEnabled(int idx, bool enabled) = 0;
    virtual void removeItem(int idx) = 0;
    virtual void removeAllItems() = 0;
    //! The highlighted item, or -1.
    virtual int getSelectedItem() = 0;
    virtual int getItemCommandId(int idx) = 0;
    virtual void setItemCommandId(int idx, int id) = 0;
    virtual IGUIContextMenu* getSubMenu(int idx) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
