// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIToolBar.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUITOOLBAR_H
#define DAISY_GUI_CGUITOOLBAR_H

#include "ox/gui/IGUIToolBar.h"

namespace daisy {
namespace gui {

//! Stays at the top of its parent like the menu bar and contains tool buttons.
class CGUIToolBar : public ox::gui::IGUIToolBar
{
public:
    CGUIToolBar(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle);

    ~CGUIToolBar();

    //! draws the element and its children
    virtual void draw();

    //! Updates the absolute position.
    virtual void updateAbsolutePosition();

    //! Adds a button to the tool bar
    virtual ox::gui::IGUIButton* addButton(int id = -1, const wchar_t* text = 0, ox::video::ITexture* img = 0,
        ox::video::ITexture* pressed = 0, bool isPushButton = false);

private:
    int ButtonX;
};

} // end namespace gui
} // end namespace daisy

#endif
