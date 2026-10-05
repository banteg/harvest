// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUILISTBOX_H
#define DAISY_GUI_CGUILISTBOX_H

#include "ox/gui/IGUIListBox.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIListBox : public ox::gui::IGUIListBox
{
public:
    CGUIListBox(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id, ox::core::CRect<int> rectangle, bool clip, bool drawBack, bool moveOverSelect);
    virtual ~CGUIListBox();
    virtual int getItemCount();
    virtual ox::gui::IGUIElement* getListItem(int index);
    virtual ox::gui::IGUIElement* getListParent();
    virtual void recalculateItemHeight();
    virtual void sortItems(bool descending);
    virtual void setItemSpacing(int spacing);
    virtual ox::gui::IGUIElement* addTextItem(const wchar_t* text, ox::gui::IGUIFont* font, ox::video::SColor color, bool selectable, bool hoverable);
    virtual void setTextItemIndent(const wchar_t* indent);
    virtual void clear();
    virtual void setSelectable(bool selectable);
    virtual int getSelected();
    virtual void setSelected(int index);
    virtual void removeItem(int index);
    virtual bool selectionWasDoubleClicked();
    virtual ox::gui::IGUIElement* getScrollBar();
    virtual void setIconFont(ox::gui::IGUIFont* font);

private:
    char Unrecovered[0x120 - sizeof(ox::gui::IGUIListBox)];
};

} // end namespace gui
} // end namespace daisy

#endif
