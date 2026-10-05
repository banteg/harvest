// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUIWINDOW_H
#define DAISY_GUI_CGUIWINDOW_H

#include "ox/gui/IGUIWindow.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIWindow : public ox::gui::IGUIWindow
{
public:
    CGUIWindow(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id, ox::core::CRect<int> rectangle, bool frame);
    virtual ~CGUIWindow();
    virtual ox::gui::IGUIButton* getCloseButton();
    virtual ox::gui::IGUIButton* getMinimizeButton();
    virtual ox::gui::IGUIButton* getMaximizeButton();
    virtual void setAnimations(ox::video::ISpritePackage* package, const char* animation);

private:
    char Unrecovered[0x218 - sizeof(ox::gui::IGUIWindow)];
};

} // end namespace gui
} // end namespace daisy

#endif
