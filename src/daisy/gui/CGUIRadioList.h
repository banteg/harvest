// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUIRADIOLIST_H
#define DAISY_GUI_CGUIRADIOLIST_H

#include "ox/gui/IGUIRadioList.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIRadioList : public ox::gui::IGUIRadioList
{
public:
    CGUIRadioList(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, ox::core::CRect<int> rectangle,
        int id);
    virtual ~CGUIRadioList();

private:
    char Unrecovered[0xd8 - sizeof(ox::gui::IGUIRadioList)];
};

} // end namespace gui
} // end namespace daisy

#endif
