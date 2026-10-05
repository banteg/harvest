// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CCLICKAREA_H
#define DAISY_GUI_CCLICKAREA_H

#include "ox/gui/IGUILayout.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CClickArea : public ox::gui::IGUILayout
{
public:
    CClickArea(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle);
    virtual ~CClickArea();

private:
    char Unrecovered[0xb8 - sizeof(ox::gui::IGUILayout)];
};

} // end namespace gui
} // end namespace daisy

#endif
