// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUICOMBOBOX_H
#define DAISY_GUI_CGUICOMBOBOX_H

#include "ox/gui/IGUIComboBox.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIComboBox : public ox::gui::IGUIComboBox
{
public:
    CGUIComboBox(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle);
    virtual ~CGUIComboBox();

private:
    char Unrecovered[0x118 - sizeof(ox::gui::IGUIComboBox)];
};

} // end namespace gui
} // end namespace daisy

#endif
