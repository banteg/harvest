// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUICheckBox.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. The virtual order follows the
// Mac 1.18 vtable of daisy::gui::CGUICheckBox.

#ifndef OX_GUI_IGUICHECKBOX_H
#define OX_GUI_IGUICHECKBOX_H

#include "IGUIElement.h"
#include "../video/SColor.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

class IGUIFont;

class IGUICheckBox : public IGUIElement
{
public:
    virtual void setChecked(bool checked) = 0;
    virtual bool isChecked() = 0;
    //! Draws the box with the named animation of the package instead of the skin.
    virtual void setAnimations(video::ISpritePackage* package, const char* name) = 0;
    virtual void setTextColor(video::SColor color) = 0;
    virtual void setTextFont(IGUIFont* font) = 0;
    virtual void updateWidth() = 0;
};

} // end namespace gui
} // end namespace ox

#endif
