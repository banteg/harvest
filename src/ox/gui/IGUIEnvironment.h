// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIEnvironment.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. Partial: the virtual order
// follows the Mac 1.18 vtable of daisy::gui::CGUIEnvironment up to addMessageBox.

#ifndef OX_GUI_IGUIENVIRONMENT_H
#define OX_GUI_IGUIENVIRONMENT_H

#include "../IUnknown.h"
#include "../core/CPosition2d.h"
#include "../core/CRect.h"
#include "../video/SColor.h"
#include "../event/IEventReceiver.h"

namespace ox {
namespace video { class IVideoDriver; }
namespace gui {

class IGUIElement;
class IGUIButton;
class IGUILayout;
class IGUISkin;
class IGUIFont;

//! Buttons of a message box.
enum EMESSAGE_BOX_FLAG
{
    EMBF_OK = 0x1,
    EMBF_CANCEL = 0x2,
    EMBF_YES = 0x4,
    EMBF_NO = 0x8
};

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
    virtual IGUIElement* getRootGUIElement() = 0;
    virtual IGUIElement* getHoverParentElement() = 0;
    virtual IGUIElement* addModalScreen() = 0;
    virtual IGUIButton* addButton(const core::CRect<int>& rectangle, IGUIElement* parent, int id,
        const wchar_t* text) = 0;
    virtual IGUIButton* addTextButton(const wchar_t* text, const char* sprite, char* hoverSprite,
        IGUIFont* font, video::SColor color, IGUIElement* parent, int id) = 0;
    virtual IGUILayout* addWindow(const core::CRect<int>& rectangle, bool modal, const wchar_t* text,
        IGUIElement* parent, int id) = 0;
    virtual IGUILayout* addFrame(const core::CRect<int>& rectangle, IGUIElement* parent, int id) = 0;
    virtual IGUILayout* addDetachableFrame(const core::CRect<int>& rectangle, IGUIElement* parent, int id,
        unsigned int flags, bool detached, bool visible) = 0;
    virtual IGUIElement* addLayoutGroup(const core::CRect<int>& rectangle, IGUIElement* parent) = 0;
    virtual void addHoverDescription(IGUIElement* element, const wchar_t* text, video::SColor* color) = 0;
    virtual IGUIElement* addMessageBox(const wchar_t* caption, const wchar_t* text, bool modal, int flags,
        IGUIElement* parent, int id) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
