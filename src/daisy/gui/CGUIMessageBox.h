// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUIMESSAGEBOX_H
#define DAISY_GUI_CGUIMESSAGEBOX_H

#include "CGUIWindow.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIMessageBox : public CGUIWindow
{
public:
    CGUIMessageBox(ox::gui::IGUIEnvironment* environment, const wchar_t* caption, const wchar_t* text, int flags,
        ox::gui::IGUIElement* parent, int id, ox::core::CRect<int> rectangle);
    virtual ~CGUIMessageBox();

private:
    char Unrecovered[0x250 - sizeof(CGUIWindow)];
};

} // end namespace gui
} // end namespace daisy

#endif
