// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIEditBox; return types that no
// recovered code uses are not verified.

#ifndef OX_GUI_IGUIEDITBOX_H
#define OX_GUI_IGUIEDITBOX_H

#include "IGUIElement.h"
#include "../video/SColor.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

class IGUIFont;

//! A single line text input.
class IGUIEditBox : public IGUIElement
{
public:
    virtual void setFocus() = 0;
    virtual void setOverrideFont(IGUIFont* font) = 0;
    virtual void setOverrideColor(video::SColor color) = 0;
    virtual void enableOverrideColor(bool enable) = 0;
    virtual void setMax(int max) = 0;
    virtual int getMax() = 0;
    virtual void setHidden(bool hidden) = 0;
    virtual void setAnimations(video::ISpritePackage* package, const char* name) = 0;
    //! The button whose click is sent when Enter is pressed in the box.
    virtual void setAssociatedButton(int id) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
