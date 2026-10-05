// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUIMENU_H
#define DAISY_GUI_CGUIMENU_H

#include "CGUIContextMenu.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIMenu : public CGUIContextMenu
{
public:
    CGUIMenu(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id, ox::core::CRect<int> rectangle);
    virtual ~CGUIMenu();
};

} // end namespace gui
} // end namespace daisy

#endif
