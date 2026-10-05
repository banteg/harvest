// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the constructor and the size are recovered here, for the widgets that create
// check boxes. The CGUICheckBox unit owns the full declaration.

#ifndef DAISY_GUI_CGUICHECKBOX_H
#define DAISY_GUI_CGUICHECKBOX_H

#include "ox/gui/IGUICheckBox.h"

namespace daisy {
namespace gui {

//! A check box drawn with the skin's sprites.
class CGUICheckBox : public ox::gui::IGUICheckBox
{
public:
    CGUICheckBox(bool checked, ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle);
    ~CGUICheckBox();

    virtual void setChecked(bool checked);
    virtual bool isChecked();
    virtual void setAnimations(ox::video::ISpritePackage* package, const char* name);
    virtual void setTextColor(ox::video::SColor color);
    virtual void setTextFont(ox::gui::IGUIFont* font);
    virtual void updateWidth();

private:
    // The Linux object is 0x100 bytes (operator new in CGUIRadioList).
    char Unrecovered[0x100 - sizeof(ox::gui::IGUICheckBox)];
};

} // end namespace gui
} // end namespace daisy

#endif
