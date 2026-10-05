// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUIMODALSCREEN_H
#define DAISY_GUI_CGUIMODALSCREEN_H

#include "ox/gui/IGUIModalScreen.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIModalScreen : public ox::gui::IGUIModalScreen
{
public:
    CGUIModalScreen(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id);
    virtual ~CGUIModalScreen();
    virtual ox::event::SEvent getLastBlockedEvent();

private:
    char Unrecovered[0xe0 - sizeof(ox::gui::IGUIModalScreen)];
};

} // end namespace gui
} // end namespace daisy

#endif
