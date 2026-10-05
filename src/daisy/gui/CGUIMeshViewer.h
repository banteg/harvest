// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUIMESHVIEWER_H
#define DAISY_GUI_CGUIMESHVIEWER_H

#include "ox/gui/IGUIElement.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIMeshViewer : public ox::gui::IGUIElement
{
public:
    CGUIMeshViewer(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle);
    virtual ~CGUIMeshViewer();

private:
    char Unrecovered[0xf8 - sizeof(ox::gui::IGUIElement)];
};

} // end namespace gui
} // end namespace daisy

#endif
