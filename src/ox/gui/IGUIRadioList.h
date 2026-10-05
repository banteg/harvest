// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIRadioList.

#ifndef OX_GUI_IGUIRADIOLIST_H
#define OX_GUI_IGUIRADIOLIST_H

#include "IGUIElement.h"
#include "../video/SColor.h"

namespace ox {
namespace gui {

class IGUICheckBox;

//! A column of check boxes of which one is checked.
class IGUIRadioList : public IGUIElement
{
public:
    IGUIRadioList(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
        Type = EGUIET_RADIO_LIST;
    }

    //! Adds a button below the others; the first one is checked.
    virtual IGUICheckBox* addRadioButton(const wchar_t* text) = 0;
    virtual int getRadioCount() = 0;
    virtual IGUICheckBox* getRadioButton(int index) = 0;
    virtual void checkRadioButton(int index) = 0;
    //! The index of the checked button.
    virtual int getSelection() = 0;
    virtual void setTextColor(video::SColor color) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
