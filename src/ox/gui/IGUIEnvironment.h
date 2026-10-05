// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIEnvironment.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. Partial: the virtual order
// follows the Mac 1.18 vtable of daisy::gui::CGUIEnvironment up to getMousePosition.

#ifndef OX_GUI_IGUIENVIRONMENT_H
#define OX_GUI_IGUIENVIRONMENT_H

#include "../IUnknown.h"
#include "../core/CPosition2d.h"
#include "../event/IEventReceiver.h"

namespace ox {
namespace video { class IVideoDriver; }
namespace gui {

class IGUIElement;
class IGUISkin;
class IGUIFont;

//! GUI Environment. Used as factory and manager of all other GUI elements.
class IGUIEnvironment : public IUnknown
{
public:
    virtual ~IGUIEnvironment() {}

    virtual void clearElements() = 0;
    virtual void drawAll() = 0;
    virtual bool setFocus(IGUIElement* element) = 0;
    virtual bool removeFocus(IGUIElement* element) = 0;
    virtual bool hasFocus(IGUIElement* element) = 0;
    virtual video::IVideoDriver* getVideoDriver() = 0;
    virtual bool postEventFromUser(event::SEvent event) = 0;
    virtual void setUserEventReceiver(event::IEventReceiver* receiver) = 0;
    virtual IGUISkin* getSkin() = 0;
    virtual IGUIFont* getFont(const char* filename) = 0;
    virtual IGUIFont* getBuiltInFont() = 0;
    virtual const core::CPosition2d<int>& getMousePosition() = 0;
};

} // end namespace gui
} // end namespace ox

#endif
