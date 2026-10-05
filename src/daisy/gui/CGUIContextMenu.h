// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIContextMenu.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Provisional: only what CGUIMenu uses. The owner of the CGUIContextMenu unit replaces this header.

#ifndef DAISY_GUI_CGUICONTEXTMENU_H
#define DAISY_GUI_CGUICONTEXTMENU_H

#include <vector>
#include "ox/gui/IGUIContextMenu.h"

namespace daisy {
namespace gui {

class CGUIContextMenu : public ox::gui::IGUIContextMenu
{
public:
    //! constructor
    CGUIContextMenu(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle, bool getFocus);

    //! destructor
    ~CGUIContextMenu();

    virtual int getItemCount() const;
    virtual int addItem(wchar_t* text, int commandId, bool enabled, bool hasSubMenu);
    virtual void addSeparator();
    virtual const wchar_t* getItemText(int index);
    virtual void setItemText(int index, const wchar_t* text);
    virtual bool isItemEnabled(int index);
    virtual void setItemEnabled(int index, bool enabled);
    virtual void removeItem(int index);
    virtual void removeAllItems();
    virtual bool OnEvent(const ox::event::SEvent& event);
    virtual void draw();
    virtual int getSelectedItem();
    virtual ox::gui::IGUIContextMenu* getSubMenu(int index);
    virtual void setVisible(bool visible);
    virtual int getItemCommandId(int index);
    virtual void setItemCommandId(int index, int id);

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

    //! highlights the item at the position; returns if an item was highlighted
    virtual bool highlight(ox::core::CPosition2d<int> p);

    //! sends a click; returns 0 if nothing was clicked, 1 for an item and 2 for a sub menu
    virtual int sendClick(ox::core::CPosition2d<int> p);

    //! returns the item highlight-area
    virtual ox::core::CRect<int> getHRect(SItem& i, ox::core::CRect<int>& absolute);

    //! returns the drawing rect of an item
    virtual ox::core::CRect<int> getRect(SItem& i, ox::core::CRect<int>& absolute);

    int HighLighted;
    std::vector<SItem> Items;
    ox::core::CPosition2d<int> Pos;
};

} // end namespace gui
} // end namespace daisy

#endif
