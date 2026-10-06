// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/ICursorControl.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. The virtual order follows the
// Mac 1.18 vtable of daisy::CIrrDeviceMacOSX's ICursorControl base.

#ifndef OX_GUI_ICURSORCONTROL_H
#define OX_GUI_ICURSORCONTROL_H

#include "../IUnknown.h"
#include "../core/CPosition2d.h"

namespace ox {
namespace gui {

//! Interface to manipulate the mouse cursor.
class ICursorControl : public IUnknown
{
public:
    virtual void setVisible(bool visible) = 0;
    virtual bool isVisible() = 0;
    //! Sets the position in coordinates relative to the window, from 0 to 1.
    virtual void setPosition(const core::CPosition2d<float>& position) = 0;
    virtual void setPosition(float x, float y) = 0;
    //! Sets the position in pixels.
    virtual void setPosition(const core::CPosition2d<int>& position) = 0;
    virtual void setPosition(int x, int y) = 0;
    //! The position in pixels.
    virtual core::CPosition2d<int> getPosition() = 0;
    //! The position relative to the window size, from 0 to 1.
    virtual core::CPosition2d<float> getRelativePosition() = 0;
    //! Whether a mouse button (0 left, 1 right, 2 middle) is held down.
    virtual bool getButtonState(int button) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
