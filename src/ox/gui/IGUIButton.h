// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIButton.

#ifndef OX_GUI_IGUIBUTTON_H
#define OX_GUI_IGUIBUTTON_H

#include "IGUIElement.h"
#include "../video/SColor.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

class IGUIFont;

//! A push button.
class IGUIButton : public IGUIElement
{
public:
    virtual void setOverrideFont(IGUIFont* font) = 0;
    virtual void setOverrideColor(const video::SColor& color) = 0;
    virtual void setAnimations(video::ISpritePackage* package, const char* name, bool animated) = 0;
    virtual void setIsPushButton(bool isPushButton) = 0;
    virtual void setPressed(bool pressed) = 0;
    virtual bool isPressed() = 0;
    virtual void fakeButtonClick() = 0;
};

} // end namespace gui
} // end namespace ox

#endif
