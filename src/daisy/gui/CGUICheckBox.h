// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUICHECKBOX_H
#define DAISY_GUI_CGUICHECKBOX_H

#include "ox/gui/IGUICheckBox.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUICheckBox : public ox::gui::IGUICheckBox
{
public:
    CGUICheckBox(bool checked, ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle);
    virtual ~CGUICheckBox();
    virtual void setChecked(bool checked);
    virtual bool isChecked();
    virtual void setAnimations(ox::video::ISpritePackage* package, const char* name);
    virtual void setTextColor(ox::video::SColor color);
    virtual void setTextFont(ox::gui::IGUIFont* font);
    virtual void updateWidth();

private:
    char Unrecovered[0x100 - sizeof(ox::gui::IGUICheckBox)];
};

} // end namespace gui
} // end namespace daisy

#endif
