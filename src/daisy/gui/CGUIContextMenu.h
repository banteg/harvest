// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIContextMenu.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUICONTEXTMENU_H
#define DAISY_GUI_CGUICONTEXTMENU_H

#include <vector>
#include "ox/gui/IGUIContextMenu.h"

namespace daisy {
namespace gui {

//! GUI Context menu.
class CGUIContextMenu : public ox::gui::IGUIContextMenu
{
public:
    //! constructor
    CGUIContextMenu(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle, bool getFocus = true);

    //! destructor
    ~CGUIContextMenu();

    //! Returns amount of menu items
    virtual int getItemCount() const;

    //! Adds a menu item.
    virtual int addItem(wchar_t* text, int commandid, bool enabled = true, bool hasSubMenu = false);

    //! Adds a separator item to the menu
    virtual void addSeparator();

    //! Returns text of the menu item.
    virtual const wchar_t* getItemText(int idx);

    //! Sets text of the menu item.
    virtual void setItemText(int idx, const wchar_t* text);

    //! Returns if a menu item is enabled
    virtual bool isItemEnabled(int idx);

    //! Sets if the menu item should be enabled.
    virtual void setItemEnabled(int idx, bool enabled);

    //! Removes a menu item
    virtual void removeItem(int idx);

    //! Removes all menu items
    virtual void removeAllItems();

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! draws the element and its children
    virtual void draw();

    //! Returns the selected item in the menu
    virtual int getSelectedItem();

    //! \return Returns a pointer to the submenu of an item.
    virtual ox::gui::IGUIContextMenu* getSubMenu(int idx);

    //! Sets the visible state of this element.
    virtual void setVisible(bool visible);

    //! Returns command id of a menu item
    virtual int getItemCommandId(int idx);

    //! Sets the command id of a menu item
    virtual void setItemCommandId(int idx, int id);

protected:
    struct SItem
    {
        ox::core::CString<wchar_t> Text;
        bool IsSeparator;
        bool Enabled;
        ox::core::CDimension2d<int> Dim;
        int PosY;
        CGUIContextMenu* SubMenu;
        int CommandId;
    };

    virtual void recalculateSize();

    //! returns true, if an element was highlighted
    virtual bool highlight(ox::core::CPosition2d<int> p);

    //! sends a click. Returns:
    //! 0 if click went outside of the element,
    //! 1 if a valid button was clicked,
    //! 2 if a nonclickable element was clicked
    virtual int sendClick(ox::core::CPosition2d<int> p);

    //! returns the item highlight-area
    virtual ox::core::CRect<int> getHRect(SItem& i, ox::core::CRect<int>& absolute);

    //! Gets drawing rect of Item
    virtual ox::core::CRect<int> getRect(SItem& i, ox::core::CRect<int>& absolute);

    int HighLighted;
    std::vector<SItem> Items;
    ox::core::CPosition2d<int> Pos;
};

} // end namespace gui
} // end namespace daisy

#endif
