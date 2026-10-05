// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUITEXTBUTTON_H
#define DAISY_GUI_CGUITEXTBUTTON_H

#include "ox/gui/IGUIElement.h"
#include "ox/gui/IGUIFont.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUITextButton : public ox::gui::IGUIElement
{
public:
    CGUITextButton(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, const wchar_t* text, int id, ox::gui::IGUIFont* font, ox::video::SColor color, const char* sprite, const char* hoverSprite);
    virtual ~CGUITextButton();

private:
    char Unrecovered[0xf8 - sizeof(ox::gui::IGUIElement)];
};

} // end namespace gui
} // end namespace daisy

#endif
