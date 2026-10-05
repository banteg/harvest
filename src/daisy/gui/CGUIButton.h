// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the constructor and the size are recovered here, for the widgets that create
// buttons. The CGUIButton unit owns the full declaration.

#ifndef DAISY_GUI_CGUIBUTTON_H
#define DAISY_GUI_CGUIBUTTON_H

#include "ox/gui/IGUIButton.h"

namespace daisy {
namespace gui {

//! A push button drawn with the skin's sprites.
class CGUIButton : public ox::gui::IGUIButton
{
public:
    CGUIButton(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle, bool noclip = false, const wchar_t* text = 0);
    ~CGUIButton();

    virtual void setOverrideFont(ox::gui::IGUIFont* font);
    virtual void setOverrideColor(const ox::video::SColor& color);
    virtual void setAnimations(ox::video::ISpritePackage* package, const char* name, bool animated);
    virtual void setIsPushButton(bool isPushButton);
    virtual void setPressed(bool pressed);
    virtual bool isPressed();
    virtual void fakeButtonClick();

private:
    // The Linux object is 0xf0 bytes (operator new in CGUIScrollBar).
    char Unrecovered[0xf0 - sizeof(ox::gui::IGUIButton)];
};

} // end namespace gui
} // end namespace daisy

#endif
