// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtables of daisy::gui::CGUIButton and CGUITextButton.

#ifndef OX_GUI_IGUIBUTTON_H
#define OX_GUI_IGUIBUTTON_H

#include "IGUIElement.h"
#include "../video/SColor.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

class IGUIFont;

//! The states of a button, each drawn with its own sprite animation.
enum EButtonStates
{
    EBS_NORMAL = 0,
    EBS_HIGHLIGHTED,
    EBS_PRESSED,
    EBS_DISABLED,
    EBS_COUNT
};

//! A push button.
class IGUIButton : public IGUIElement
{
public:
    IGUIButton(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
    }

    virtual void setOverrideFont(IGUIFont* font) = 0;
    virtual void setOverrideColor(const video::SColor& color) = 0;
    virtual void setAnimations(video::ISpritePackage* package, const char* name, bool animated) = 0;
    virtual void setIsPushButton(bool isPushButton) = 0;
    virtual void setPressed(bool pressed) = 0;
    virtual bool isPressed() = 0;
    virtual void fakeButtonClick() = 0;
};

//! A button drawn as a text with optional sprites around it.
class IGUITextButton : public IGUIElement
{
public:
    IGUITextButton(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
    }

    virtual void setTextColor(video::SColor color) = 0;
};

} // end namespace gui
} // end namespace ox

#endif
