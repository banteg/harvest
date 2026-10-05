// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIInOutFader.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. The virtual order follows the
// Mac 1.18 vtable of daisy::gui::CGUIInOutFader.

#ifndef OX_GUI_IGUIINOUTFADER_H
#define OX_GUI_IGUIINOUTFADER_H

#include "IGUIElement.h"
#include "../video/SColor.h"

namespace ox {
namespace gui {

//! Element for fading out or in.
class IGUIInOutFader : public IGUIElement
{
public:
    IGUIInOutFader(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
    }

    //! Gets the color to fade out to or to fade in from.
    virtual video::SColor getColor() const = 0;
    //! Sets the color to fade out to or to fade in from.
    virtual void setColor(video::SColor color) = 0;
    //! Starts the fade in process.
    virtual void fadeIn(unsigned int time) = 0;
    //! Starts the fade out process.
    virtual void fadeOut(unsigned int time) = 0;
    //! Returns if the fade in or out process is done.
    virtual bool isReady() const = 0;
};

} // end namespace gui
} // end namespace ox

#endif
