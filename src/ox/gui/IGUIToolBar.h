// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIToolbar.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. The virtual order follows the
// Mac 1.18 vtable of daisy::gui::CGUIToolBar.

#ifndef OX_GUI_IGUITOOLBAR_H
#define OX_GUI_IGUITOOLBAR_H

#include "IGUIElement.h"

namespace ox {
namespace video { class ITexture; }
namespace gui {

class IGUIButton;

//! Stays at the top of its parent like the menu bar and contains tool buttons.
class IGUIToolBar : public IGUIElement
{
public:
    IGUIToolBar(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
    }

    //! Adds a button to the tool bar. The images are ignored.
    virtual IGUIButton* addButton(int id = -1, const wchar_t* text = 0, video::ITexture* img = 0,
        video::ITexture* pressed = 0, bool isPushButton = false) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
