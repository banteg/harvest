// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIModalScreen.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUIMODALSCREEN_H
#define DAISY_GUI_CGUIMODALSCREEN_H

#include "ox/gui/IGUIModalScreen.h"

namespace daisy {
namespace gui {

//! Covers its parent, tints what lies behind it and swallows the mouse and key events meant for it.
class CGUIModalScreen : public ox::gui::IGUIModalScreen
{
public:
    //! constructor
    CGUIModalScreen(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id);

    //! destructor
    ~CGUIModalScreen();

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! draws the element and its children
    virtual void draw();

    //! Removes a child; the screen removes itself with its last child.
    virtual void removeChild(ox::gui::IGUIElement* child);

    //! Updates the absolute position.
    virtual void updateAbsolutePosition();

    //! The last mouse or key event the screen blocked.
    virtual ox::event::SEvent getLastBlockedEvent();

private:
    ox::event::SEvent LastBlockedEvent;
};

} // end namespace gui
} // end namespace daisy

#endif
