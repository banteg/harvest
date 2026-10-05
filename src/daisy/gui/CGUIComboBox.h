// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIComboBox.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye draws the box with sprite animations and opens the list on the root element.

#ifndef DAISY_GUI_CGUICOMBOBOX_H
#define DAISY_GUI_CGUICOMBOBOX_H

#include "ox/gui/IGUIComboBox.h"
#include "ox/TArray.h"

namespace ox {
namespace gui {
class IGUIButton;
class IGUIListBox;
} // end namespace gui
namespace video { class ISpriteAnimationState; }
} // end namespace ox

namespace daisy {
namespace gui {

//! A combo box drawn with a background and a left end animation, or with skin colors.
class CGUIComboBox : public ox::gui::IGUIComboBox
{
public:
    //! constructor
    CGUIComboBox(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle);

    //! destructor
    virtual ~CGUIComboBox();

    //! moves the button to the right end and keeps the button's height
    virtual void setRelativePosition(const ox::core::CRect<int>& position);

    //! Returns amount of items in box
    virtual int getItemCount();

    //! returns string of an item. the idx may be a value from 0 to itemCount-1
    virtual const wchar_t* getItem(int index);

    //! adds an item and returns the index of it
    virtual int addItem(const wchar_t* text);

    //! deletes all items in the combo box
    virtual void clear();

    //! returns id of selected item. returns -1 if no item is selected.
    virtual int getSelected();

    //! sets the selected item. Set this to -1 if no item should be selected
    virtual void setSelected(int index);

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! draws the element and its children
    virtual void draw();

    //! starts the animations "<name>Background" and "<name>Left"
    virtual void setAnimations(ox::video::ISpritePackage* package, const char* name);

private:
    //! The animations, in the order of COMBOBOX_ANIMATION_NAMES.
    enum EAnimation
    {
        EA_BACKGROUND = 0,
        EA_LEFT,
        EA_COUNT
    };

    void openCloseMenu();

    ox::gui::IGUIButton* ListButton;
    ox::gui::IGUIListBox* ListBox;
    ox::TArray<ox::core::CString<wchar_t> > Items;
    int Selected;
    ox::video::ISpriteAnimationState* Animations[EA_COUNT];
    //! The frame rectangle of each animation, relative to the element.
    ox::core::CRect<int> AnimationRects[EA_COUNT];
};

} // end namespace gui
} // end namespace daisy

#endif
