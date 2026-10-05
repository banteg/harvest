// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIContextMenu.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. The virtual order follows the
// Mac 1.18 vtable of daisy::gui::CGUIContextMenu.

#ifndef OX_GUI_IGUICONTEXTMENU_H
#define OX_GUI_IGUICONTEXTMENU_H

#include "IGUIElement.h"

namespace ox {
namespace gui {

//! A context menu, or a menu bar through daisy::gui::CGUIMenu.
class IGUIContextMenu : public IGUIElement
{
public:
    IGUIContextMenu(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
    }

    virtual int getItemCount() const = 0;
    //! Adds an item; hasSubMenu creates a sub menu for it. Returns the index of the item.
    virtual int addItem(wchar_t* text, int commandId, bool enabled, bool hasSubMenu) = 0;
    virtual void addSeparator() = 0;
    virtual const wchar_t* getItemText(int index) = 0;
    virtual void setItemText(int index, const wchar_t* text) = 0;
    virtual bool isItemEnabled(int index) = 0;
    virtual void setItemEnabled(int index, bool enabled) = 0;
    virtual void removeItem(int index) = 0;
    virtual void removeAllItems() = 0;
    virtual int getSelectedItem() = 0;
    virtual int getItemCommandId(int index) = 0;
    virtual void setItemCommandId(int index, int id) = 0;
    virtual IGUIContextMenu* getSubMenu(int index) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
