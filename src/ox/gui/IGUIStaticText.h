// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIStaticText.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. The virtual order follows the
// Mac 1.18 vtable of daisy::gui::CGUIStaticText.

#ifndef OX_GUI_IGUISTATICTEXT_H
#define OX_GUI_IGUISTATICTEXT_H

#include "IGUIElement.h"
#include "IGUIFont.h"
#include "../video/SColor.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

//! A static text.
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
    //! Shows a sprite before the text.
    virtual void setParagraphIcon(const char* sprite, video::ISpritePackage* package, bool animated) = 0;
    //! Reveals the text a character at a time over the given milliseconds; 0 shows it at once.
    virtual void activateProgressiveReveal(unsigned int time) = 0;
    virtual unsigned int getTotalProgressiveTime() = 0;
    virtual void activateOffsetScrollingToEnsureVisibleText() = 0;
    virtual void packSize() = 0;
};

} // end namespace gui
} // end namespace ox

#endif
