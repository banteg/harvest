// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUIFILEOPENDIALOG_H
#define DAISY_GUI_CGUIFILEOPENDIALOG_H

#include "ox/gui/IGUIFileOpenDialog.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIFileOpenDialog : public ox::gui::IGUIFileOpenDialog
{
public:
    CGUIFileOpenDialog(ox::io::IFileSystem* fileSystem, const wchar_t* title, ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id, const char* directory, const char* filter);
    virtual ~CGUIFileOpenDialog();

private:
    char Unrecovered[0x128 - sizeof(ox::gui::IGUIFileOpenDialog)];
};

} // end namespace gui
} // end namespace daisy

#endif
