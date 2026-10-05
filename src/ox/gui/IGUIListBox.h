// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIListBox.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. The virtual order follows the
// Mac 1.18 vtable of daisy::gui::CGUIListBox; the list items are GUI elements of their own.

#ifndef OX_GUI_IGUILISTBOX_H
#define OX_GUI_IGUILISTBOX_H

#include "IGUIElement.h"
#include "../video/SColor.h"

namespace ox {
namespace gui {

class IGUIFont;
class IGUIScrollBar;

//! A scrolling list of elements.
class IGUIListBox : public IGUIElement
{
public:
    virtual int getItemCount() = 0;
    virtual IGUIElement* getListItem(int index) = 0;
    //! The element new items are added to.
    virtual IGUIElement* getListParent() = 0;
    virtual void recalculateItemHeight() = 0;
    virtual void sortItems(bool resize) = 0;
    virtual void setItemSpacing(int spacing) = 0;
    virtual IGUIElement* addTextItem(const wchar_t* text, IGUIFont* font, video::SColor color, bool selectable,
        bool indent) = 0;
    virtual void setTextItemIndent(const wchar_t* indent) = 0;
    virtual void clear() = 0;
    virtual void setSelectable(bool selectable) = 0;
    virtual int getSelected() = 0;
    virtual void setSelected(int index) = 0;
    virtual void removeItem(int index) = 0;
    virtual bool selectionWasDoubleClicked() = 0;
    virtual IGUIScrollBar* getScrollBar() = 0;
    virtual void setIconFont(IGUIFont* font) = 0;

    //! Receives the events of the list items.
    event::IEventReceiver* EventReceiver;
};

} // end namespace gui
} // end namespace ox

#endif
