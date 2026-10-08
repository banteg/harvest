// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIListBox; return types other
// than getSelected's are provisional.

#ifndef OX_GUI_IGUILISTBOX_H
#define OX_GUI_IGUILISTBOX_H

#include "IGUIElement.h"
#include "../video/SColor.h"

namespace ox {
namespace gui {

class IGUIFont;

//! A list of items, which are texts or arbitrary elements.
class IGUIListBox : public IGUIElement
{
public:
    IGUIListBox(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
    }

    virtual int getItemCount() = 0;
    virtual IGUIElement* getListItem(int index) = 0;
    //! The element holding the items; elements added to it become items.
    virtual IGUIElement* getListParent() = 0;
    virtual void recalculateItemHeight() = 0;
    virtual void sortItems(bool descending) = 0;
    virtual void setItemSpacing(int spacing) = 0;
    virtual IGUIElement* addTextItem(const wchar_t* text, IGUIFont* font, video::SColor color, bool selectable,
        bool hoverable) = 0;
    virtual void setTextItemIndent(const wchar_t* indent) = 0;
    virtual void clear() = 0;
    virtual void setSelectable(bool selectable) = 0;
    virtual int getSelected() = 0;
    virtual void setSelected(int index) = 0;
    virtual void removeItem(int index) = 0;
    virtual bool selectionWasDoubleClicked() = 0;
    virtual IGUIElement* getScrollBar() = 0;
};

} // end namespace gui
} // end namespace ox

#endif
