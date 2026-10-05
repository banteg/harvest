// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: the virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIStaticText up to
// setWordWrap.

#ifndef OX_GUI_IGUISTATICTEXT_H
#define OX_GUI_IGUISTATICTEXT_H

#include "IGUIElement.h"
#include "../video/SColor.h"

namespace ox {
namespace gui {

class IGUIFont;

//! A static text.
class IGUIStaticText : public IGUIElement
{
public:
    virtual void setOverrideFont(IGUIFont* font) = 0;
    virtual void setOverrideColor(video::SColor color) = 0;
    virtual video::SColor getOverrideColor() = 0;
    virtual void enableOverrideColor(bool enable) = 0;
    virtual void setWordWrap(bool enable) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
