// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Provisional: only the size and the constructor that daisy::gui::CGUIEnvironment's factory calls.

#ifndef DAISY_GUI_CGUISTATICTEXT_H
#define DAISY_GUI_CGUISTATICTEXT_H

#include "ox/gui/IGUIStaticText.h"
#include "ox/gui/IGUIStaticTextInline.h"

namespace ox {
class IOSOperator;
namespace io { class IFileSystem; }
namespace video { class ISpritePackage; class ITexture; }
}

namespace daisy {
namespace gui {

class CGUIStaticText : public ox::gui::IGUIStaticText
{
public:
    CGUIStaticText(const wchar_t* text, bool border, ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id, const ox::core::CRect<int>& rectangle, const wchar_t* style);
    virtual ~CGUIStaticText();
    virtual void setOverrideFont(ox::gui::IGUIFont* font);
    virtual void setOverrideColor(ox::video::SColor color);
    virtual ox::video::SColor getOverrideColor();
    virtual void enableOverrideColor(bool enable);
    virtual void setWordWrap(bool enable);
    virtual void setTextAlignment(ox::gui::EFontHorizontalAlign horizontal, ox::gui::EFontVerticalAlign vertical);
    virtual int getTextHeight();
    virtual void setParagraphIcon(const char* name, ox::video::ISpritePackage* package, bool animated);
    virtual void activateProgressiveReveal(unsigned int time);
    virtual unsigned int getTotalProgressiveTime();
    virtual void activateOffsetScrollingToEnsureVisibleText();
    virtual void packSize();

private:
    char Unrecovered[0x120 - sizeof(ox::gui::IGUIStaticText)];
};

} // end namespace gui
} // end namespace daisy

#endif
