// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUICheckBox.

#ifndef OX_GUI_IGUICHECKBOX_H
#define OX_GUI_IGUICHECKBOX_H

#include "IGUIElement.h"
#include "../video/SColor.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

class IGUIFont;

//! A check box with a text.
class IGUICheckBox : public IGUIElement
{
public:
    IGUICheckBox(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
    }

    virtual void setChecked(bool checked) = 0;
    virtual bool isChecked() = 0;
    virtual void setAnimations(video::ISpritePackage* package, const char* name) = 0;
    virtual void setTextColor(video::SColor color) = 0;
    virtual void setTextFont(IGUIFont* font) = 0;
    virtual void updateWidth() = 0;
};

} // end namespace gui
} // end namespace ox

#endif
