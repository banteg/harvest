// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIStaticText; return types that no
// recovered code uses are not verified.

#ifndef OX_GUI_IGUISTATICTEXT_H
#define OX_GUI_IGUISTATICTEXT_H

#include "IGUIElement.h"
#include "IGUIFont.h"
#include "../video/SColor.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

//! A text label, optionally word wrapped, with an icon and a progressive reveal.
class IGUIStaticText : public IGUIElement
{
public:
    virtual void setOverrideFont(IGUIFont* font) = 0;
    virtual void setOverrideColor(video::SColor color) = 0;
    virtual video::SColor getOverrideColor() = 0;
    virtual void enableOverrideColor(bool enable) = 0;
    virtual void setWordWrap(bool enable) = 0;
    virtual void setTextAlignment(EFontHorizontalAlign horizontal, EFontVerticalAlign vertical) = 0;
    virtual int getTextHeight() = 0;
    //! Shows the named sprite of the package before the first paragraph.
    virtual void setParagraphIcon(const char* name, video::ISpritePackage* package, bool enable) = 0;
    //! Reveals the text a character at a time, starting after a delay.
    virtual void activateProgressiveReveal(unsigned int delay) = 0;
    virtual int getTotalProgressiveTime() = 0;
    virtual void activateOffsetScrollingToEnsureVisibleText() = 0;
    virtual void packSize() = 0;
};

} // end namespace gui
} // end namespace ox

#endif
