// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIListBox; return types that no
// recovered code uses are not verified.

#ifndef OX_GUI_IGUILISTBOX_H
#define OX_GUI_IGUILISTBOX_H

#include "IGUIElement.h"
#include "../video/SColor.h"

namespace ox {
namespace gui {

class IGUIFont;
class IGUIScrollBar;

//! A list of elements; items are the children of its list parent.
class IGUIListBox : public IGUIElement
{
public:
    virtual int getItemCount() = 0;
    virtual IGUIElement* getListItem(int index) = 0;
    //! The element that list items are added to.
    virtual IGUIElement* getListParent() = 0;
    virtual void recalculateItemHeight() = 0;
    virtual void sortItems(bool resize) = 0;
    virtual void setItemSpacing(int spacing) = 0;
    virtual IGUIElement* addTextItem(const wchar_t* text, IGUIFont* font, video::SColor color, bool selectable,
        bool indent) = 0;
    virtual void setTextItemIndent(const wchar_t* indent) = 0;
    virtual void clear() = 0;
    virtual void setSelectable(bool selectable) = 0;
    //! The index of the selected item, or -1.
    virtual int getSelected() = 0;
    virtual void setSelected(int index) = 0;
    virtual void removeItem(int index) = 0;
    virtual bool selectionWasDoubleClicked() = 0;
    virtual IGUIScrollBar* getScrollBar() = 0;
    virtual void setIconFont(IGUIFont* font) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
