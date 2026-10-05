// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUIEDITBOX_H
#define DAISY_GUI_CGUIEDITBOX_H

#include "ox/gui/IGUIEditBox.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIEditBox : public ox::gui::IGUIEditBox
{
public:
    CGUIEditBox(const wchar_t* text, bool border, ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent,
        int id, const ox::core::CRect<int>& rectangle, ox::IOSOperator* op);
    virtual ~CGUIEditBox();
    virtual void setFocus();
    virtual void setOverrideFont(ox::gui::IGUIFont* font);
    virtual void setOverrideColor(ox::video::SColor color);
    virtual void enableOverrideColor(bool enable);
    virtual void setMax(int max);
    virtual int getMax();
    virtual void setHidden(bool hidden);
    virtual void setAnimations(ox::video::ISpritePackage* package, const char* name);
    virtual void setAssociatedButton(int id);

private:
    char Unrecovered[0x138 - sizeof(ox::gui::IGUIEditBox)];
};

} // end namespace gui
} // end namespace daisy

#endif
