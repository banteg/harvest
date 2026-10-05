// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUIBUTTON_H
#define DAISY_GUI_CGUIBUTTON_H

#include "ox/gui/IGUIButton.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIButton : public ox::gui::IGUIButton
{
public:
    CGUIButton(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle, bool noClip, const wchar_t* text);
    virtual ~CGUIButton();
    virtual void setOverrideFont(ox::gui::IGUIFont* font);
    virtual void setOverrideColor(const ox::video::SColor& color);
    virtual void setAnimations(ox::video::ISpritePackage* package, const char* name, bool animated);
    virtual void setIsPushButton(bool isPushButton);
    virtual void setPressed(bool pressed);
    virtual bool isPressed();
    virtual void fakeButtonClick();

private:
    char Unrecovered[0xf0 - sizeof(ox::gui::IGUIButton)];
};

} // end namespace gui
} // end namespace daisy

#endif
