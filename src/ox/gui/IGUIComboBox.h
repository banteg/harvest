// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIComboBox.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. The virtual order follows the
// Mac 1.18 vtable of daisy::gui::CGUIComboBox.

#ifndef OX_GUI_IGUICOMBOBOX_H
#define OX_GUI_IGUICOMBOBOX_H

#include "IGUIElement.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

//! A combo box: a selected text and a button that opens the list of all texts.
class IGUIComboBox : public IGUIElement
{
public:
    IGUIComboBox(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
    }

    //! Returns amount of items in box
    virtual int getItemCount() = 0;
    //! returns string of an item. the idx may be a value from 0 to itemCount-1
    virtual const wchar_t* getItem(int index) = 0;
    //! adds an item and returns the index of it
    virtual int addItem(const wchar_t* text) = 0;
    //! deletes all items in the combo box
    virtual void clear() = 0;
    //! returns id of selected item. returns -1 if no item is selected.
    virtual int getSelected() = 0;
    //! sets the selected item. Set this to -1 if no item should be selected
    virtual void setSelected(int index) = 0;
    virtual void setAnimations(video::ISpritePackage* package, const char* name) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
