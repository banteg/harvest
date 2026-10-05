// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIListBox.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUILISTBOX_H
#define DAISY_GUI_CGUILISTBOX_H

#include "ox/gui/IGUIListBox.h"

namespace ox {
namespace gui {
class IGUILayout;
class IGUIScrollBar;
} // end namespace gui
} // end namespace ox

namespace daisy {
namespace gui {

//! A list of elements placed in a scrolled layout group. Text items are static texts.
class CGUIListBox : public ox::gui::IGUIListBox
{
public:
    //! constructor
    CGUIListBox(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle, bool clip, bool drawBack, bool moveOverSelect);

    //! destructor
    ~CGUIListBox();

    virtual void setRelativePosition(const ox::core::CRect<int>& position);

    //! returns amount of list items
    virtual int getItemCount();

    //! returns the list item at an index
    virtual ox::gui::IGUIElement* getListItem(int index);

    //! returns the element holding the items
    virtual ox::gui::IGUIElement* getListParent();

    //! updates the item height and the scroll bar range from the list size
    virtual void recalculateItemHeight();

    //! stacks the items vertically; scrollToEnd shows the last items unless the scroll bar is dragged
    virtual void sortItems(bool scrollToEnd);

    //! sets the space between items
    virtual void setItemSpacing(int spacing);

    //! adds a static text item; wordWrap breaks it into lines indented by the text item indent
    virtual ox::gui::IGUIElement* addTextItem(const wchar_t* text, ox::gui::IGUIFont* font,
        ox::video::SColor color, bool scrollToEnd, bool wordWrap);

    //! sets the indent of the wrapped lines of text items
    virtual void setTextItemIndent(const wchar_t* indent);

    //! clears the list
    virtual void clear();

    virtual void setSelectable(bool selectable);

    //! returns id of selected item. returns -1 if no item is selected.
    virtual int getSelected();

    //! sets the selected item. Set this to -1 if no item should be selected
    virtual void setSelected(int index);

    virtual void removeItem(int index);

    //! returns if the last selection selected the selected item again with a double click
    virtual bool selectionWasDoubleClicked();

    virtual ox::gui::IGUIElement* getScrollBar();

    //! Sets the font which should be used as icon font.
    virtual void setIconFont(ox::gui::IGUIFont* font);

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! draws the element and its children
    virtual void draw();

    //! sets the element that receives the list events, the parent by default
    void setOverrideActionParent(ox::gui::IGUIElement* parent);

private:
    //! selects the item under a point and reports it unless onlyHover is set
    bool selectNew(int x, int y, bool onlyHover, int clicks);

    int Selected;
    //! The item the last selection event reported.
    int LastSelected;
    //! The relative y of the list parent when scrolled to the top.
    int ListParentY;
    int ItemHeight;
    int TotalItemHeight;
    ox::gui::IGUIFont* Font;
    ox::gui::IGUIFont* IconFont;
    bool Selecting;
    int ItemSpacing;
    ox::core::CString<wchar_t> TextItemIndent;
    ox::gui::IGUIScrollBar* ScrollBar;
    //! The frame or layout group holding the list parent and the scroll bar.
    ox::gui::IGUILayout* Frame;
    ox::gui::IGUILayout* ListParent;
    bool Clip;
    bool DrawBack;
    bool MoveOverSelect;
    //! Set when a click went to the scroll bar.
    bool ScrollBarClicked;
    bool Selectable;
    bool SelectedAgain;
    ox::gui::IGUIElement* OverrideActionParent;
};

} // end namespace gui
} // end namespace daisy

#endif
