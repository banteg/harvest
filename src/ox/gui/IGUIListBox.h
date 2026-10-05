// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIListBox.

#ifndef OX_GUI_IGUILISTBOX_H
#define OX_GUI_IGUILISTBOX_H

#include "IGUIElement.h"
#include "../video/SColor.h"

namespace ox {
namespace gui {

class IGUIFont;
class IGUIScrollBar;

//! A list of selectable items.
class IGUIListBox : public IGUIElement
{
public:
    virtual int getItemCount() = 0;
    virtual IGUIElement* getListItem(int index) = 0;
    virtual IGUIElement* getListParent() = 0;
    virtual void recalculateItemHeight() = 0;
    virtual void sortItems(bool ascending) = 0;
    virtual void setItemSpacing(int spacing) = 0;
    //! The meaning of the two flags is not recovered.
    virtual IGUIElement* addTextItem(const wchar_t* text, IGUIFont* font, video::SColor color, bool, bool) = 0;
    virtual core::CString<wchar_t>& setTextItemIndent(const wchar_t* indent) = 0;
    virtual void clear() = 0;
    virtual void setSelectable(bool selectable) = 0;
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
