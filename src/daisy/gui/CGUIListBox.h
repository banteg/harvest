// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the constructor, the method and the size that CGUIComboBox needs. The owner of
// the CGUIListBox unit replaces this header.

#ifndef DAISY_GUI_CGUILISTBOX_H
#define DAISY_GUI_CGUILISTBOX_H

#include "ox/gui/IGUIListBox.h"

namespace daisy {
namespace gui {

class CGUIListBox : public ox::gui::IGUIListBox
{
public:
    CGUIListBox(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle, bool clip, bool drawBack, bool moveOverSelect);

    virtual ~CGUIListBox();

    virtual int getItemCount();
    virtual ox::gui::IGUIElement* getListItem(int index);
    virtual ox::gui::IGUIElement* getListParent();
    virtual void recalculateItemHeight();
    virtual void sortItems(bool descending);
    virtual void setItemSpacing(int spacing);
    virtual ox::gui::IGUIElement* addTextItem(const wchar_t* text, ox::gui::IGUIFont* font, ox::video::SColor color,
        bool selectable, bool hoverable);
    virtual void setTextItemIndent(const wchar_t* indent);
    virtual void clear();
    virtual void setSelectable(bool selectable);
    virtual int getSelected();
    virtual void setSelected(int index);
    virtual void removeItem(int index);
    virtual bool selectionWasDoubleClicked();
    virtual ox::gui::IGUIElement* getScrollBar();
    virtual void setIconFont(ox::gui::IGUIFont* font);

    //! Sends the list's events to another element than the parent.
    void setOverrideActionParent(ox::gui::IGUIElement* parent);

private:
    char Unrecovered[0x120 - sizeof(ox::gui::IGUIListBox)];
};

} // end namespace gui
} // end namespace daisy

#endif
