// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUIPOPUPMENU_H
#define DAISY_GUI_CGUIPOPUPMENU_H

#include "ox/gui/IGUIPopupMenu.h"
#include "ox/gui/IGUIFont.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIPopupMenu : public ox::gui::IGUIPopupMenu
{
public:
    CGUIPopupMenu(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CPosition2d<int> position, int width, const wchar_t* text, ox::gui::IGUIFont* font,
            ox::gui::IGUIFont* hoverFont);
    virtual ~CGUIPopupMenu();

private:
    char Unrecovered[0xe0 - sizeof(ox::gui::IGUIPopupMenu)];
};

} // end namespace gui
} // end namespace daisy

#endif
