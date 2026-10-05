// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUIINOUTFADER_H
#define DAISY_GUI_CGUIINOUTFADER_H

#include "ox/gui/IGUIElement.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIInOutFader : public ox::gui::IGUIElement
{
public:
    CGUIInOutFader(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id, ox::core::CRect<int> rectangle);
    virtual ~CGUIInOutFader();

private:
    char Unrecovered[0xd0 - sizeof(ox::gui::IGUIElement)];
};

} // end namespace gui
} // end namespace daisy

#endif
