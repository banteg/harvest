// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/ICursorControl.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. Partial: only the visibility
// methods are recovered.

#ifndef OX_GUI_ICURSORCONTROL_H
#define OX_GUI_ICURSORCONTROL_H

#include "../IUnknown.h"

namespace ox {
namespace gui {

//! Interface to manipulate the mouse cursor.
class ICursorControl : public IUnknown
{
public:
    virtual void setVisible(bool visible) = 0;
    virtual bool isVisible() = 0;
};

} // end namespace gui
} // end namespace ox

#endif
