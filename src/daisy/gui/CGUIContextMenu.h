// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUICONTEXTMENU_H
#define DAISY_GUI_CGUICONTEXTMENU_H

#include "ox/gui/IGUIContextMenu.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIContextMenu : public ox::gui::IGUIContextMenu
{
public:
    CGUIContextMenu(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id, ox::core::CRect<int> rectangle, bool getFocus);
    virtual ~CGUIContextMenu();

private:
    char Unrecovered[0xe0 - sizeof(ox::gui::IGUIContextMenu)];
};

} // end namespace gui
} // end namespace daisy

#endif
