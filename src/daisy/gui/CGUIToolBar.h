// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUITOOLBAR_H
#define DAISY_GUI_CGUITOOLBAR_H

#include "ox/gui/IGUIToolBar.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIToolBar : public ox::gui::IGUIToolBar
{
public:
    CGUIToolBar(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id, ox::core::CRect<int> rectangle);
    virtual ~CGUIToolBar();

private:
    char Unrecovered[0xc0 - sizeof(ox::gui::IGUIToolBar)];
};

} // end namespace gui
} // end namespace daisy

#endif
