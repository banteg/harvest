// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIButton.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye replaces the pressed flag with sprite-animated states, reports right clicks and adds
// CGUITextButton.

#include "CGUIButton.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/gui/IGUISkin.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

using namespace ox;
using namespace ox::gui;

//! The first character of a close button's text.
static const wchar_t CLOSE_BUTTON_CHARACTER = 0x103;

//! constructor
CGUIButton::CGUIButton(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle,
    bool noclip, const wchar_t* text)
    : IGUIButton(environment, parent, id, rectangle), State(EBS_NORMAL), NoClip(noclip), IsPushButton(false),
      OverrideFont(0), OverrideColorEnabled(false)
{
    Type = EGUIET_BUTTON;

    for (int i = 0; i < EBS_COUNT; ++i)
        States[i] = 0;

    if (text)
        Text = text;

    video::ISpritePackage* package = Environment->getSkin()->getSpritePackage();
    if (!package)
        return;

    if (!text)
        setAnimations(package, "Button", true);
    else if (text[0] == CLOSE_BUTTON_CHARACTER)
    {
        setAnimations(package, "CloseBtn", true);
        setText(L"");
    }
    else
        setAnimations(package, "TextButton", true);
}

//! destructor
CGUIButton::~CGUIButton()
{
    if (OverrideFont)
        OverrideFont->drop();

    for (int i = 0; i < EBS_COUNT; ++i)
        if (States[i])
            States[i]->remove();
}

//! called if an event happened.
bool CGUIButton::OnEvent(const event::SEvent& event)
{
    if (!isEnabled())
    {
        setNewState(EBS_DISABLED);
        return Parent ? Parent->OnEvent(event) : false;
    }

    switch (event.EventType)
    {
    case event::EET_KEY_INPUT_EVENT:
        if (event.KeyInput.Key == KEY_RETURN || event.KeyInput.Key == KEY_SPACE)
        {
            if (event.KeyInput.Event == event::EKIE_KEY_PRESSED_DOWN)
            {
                if (!IsPushButton || State != EBS_PRESSED)
                    setNewState(EBS_PRESSED);
                else
                    setNewState(EBS_HIGHLIGHTED);
                return true;
            }
            else if (event.KeyInput.Event == event::EKIE_KEY_LEFT_UP && State == EBS_PRESSED)
            {
                Environment->removeFocus(this);

                if (!IsPushButton)
                    setNewState(EBS_HIGHLIGHTED);

                if (Parent)
                {
                    event::SEvent e;
                    e.EventType = event::EET_GUI_EVENT;
                    e.GUIEvent.Caller = this;
                    e.GUIEvent.EventType = EGET_BUTTON_CLICKED;
                    Parent->OnEvent(e);
                }
                return true;
            }
        }
        break;
    case event::EET_GUI_EVENT:
        if (event.GUIEvent.EventType == EGET_ELEMENT_LEFT)
        {
            if (!IsPushButton || State != EBS_PRESSED)
                setNewState(EBS_NORMAL);
            return true;
        }
        else if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED)
        {
            if (!IsPushButton || State != EBS_PRESSED)
                setNewState(EBS_HIGHLIGHTED);
            return true;
        }
        break;
    case event::EET_MOUSE_INPUT_EVENT:
        if (event.MouseInput.Event == event::EMIE_LMOUSE_PRESSED_DOWN ||
            event.MouseInput.Event == event::EMIE_RMOUSE_PRESSED_DOWN)
        {
            if (Environment->hasFocus(this) &&
                !AbsoluteRect.isPointInside(core::CPosition2d<int>(event.MouseInput.X, event.MouseInput.Y)))
            {
                Environment->removeFocus(this);
                return false;
            }

            if (!IsPushButton)
                setNewState(EBS_PRESSED);

            Environment->setFocus(this);
            return true;
        }
        else if (event.MouseInput.Event == event::EMIE_LMOUSE_LEFT_UP ||
            event.MouseInput.Event == event::EMIE_RMOUSE_LEFT_UP)
        {
            EButtonStates oldState = State;
            Environment->removeFocus(this);

            if (!IsPushButton)
                setNewState(EBS_HIGHLIGHTED);
            else if (AbsoluteRect.isPointInside(core::CPosition2d<int>(event.MouseInput.X, event.MouseInput.Y)))
            {
                if (State == EBS_PRESSED)
                    setNewState(EBS_HIGHLIGHTED);
                else
                    setNewState(EBS_PRESSED);
            }

            if ((!IsPushButton && oldState == EBS_PRESSED && Parent) ||
                (Parent && IsPushButton && State != oldState))
            {
                // right clicks are reported as EGET_TEXT_BUTTON_CLICKED
                event::SEvent e;
                e.EventType = event::EET_GUI_EVENT;
                e.GUIEvent.Caller = this;
                e.GUIEvent.EventType = event.MouseInput.Event == event::EMIE_LMOUSE_LEFT_UP ?
                    EGET_BUTTON_CLICKED : EGET_TEXT_BUTTON_CLICKED;
                Parent->OnEvent(e);
            }

            return true;
        }
        break;
    default:
        break;
    }

    return Parent ? Parent->OnEvent(event) : false;
}

//! switches to a state and restarts its animation
void CGUIButton::setNewState(EButtonStates state)
{
    if (State == state)
        return;

    State = state;
    if (States[state])
        States[state]->reset();
}

//! draws the element and its children
void CGUIButton::draw()
{
    if (!IsVisible)
        return;

    if (!isEnabled() && State != EBS_DISABLED)
        setNewState(EBS_DISABLED);
    else if (isEnabled() && State == EBS_DISABLED)
        setNewState(EBS_NORMAL);

    IGUISkin* skin = Environment->getSkin();
    video::IVideoDriver* driver = Environment->getVideoDriver();

    IGUIFont* font = OverrideFont;
    if (!OverrideFont)
        font = skin->getFont();

    if (RelativeSizeChanged && States[EBS_NORMAL])
    {
        core::CDimension2d<int> size = States[EBS_NORMAL]->getFrameSize(0);
        RelativeRect.LowerRightCorner.X = RelativeRect.UpperLeftCorner.X + size.Width;
        RelativeRect.LowerRightCorner.Y = RelativeRect.UpperLeftCorner.Y + size.Height;
        updateAbsolutePosition();
    }

    core::CRect<int> rect = AbsoluteRect;
    core::CRect<int>* clip = &AbsoluteClippingRect;
    if (NoClip)
        clip = 0;

    if (State != EBS_PRESSED)
    {
        if (States[State])
            States[State]->draw(rect.UpperLeftCorner, clip, video::SColor(0xffffffff));
        else
        {
            driver->draw2DRectangle(skin->getColor(EGDC_3D_DARK_SHADOW), rect, clip);

            rect.LowerRightCorner.X -= 1;
            rect.LowerRightCorner.Y -= 1;
            driver->draw2DRectangle(skin->getColor(EGDC_3D_HIGH_LIGHT), rect, clip);

            rect.UpperLeftCorner.X += 1;
            rect.UpperLeftCorner.Y += 1;
            driver->draw2DRectangle(skin->getColor(EGDC_3D_SHADOW), rect, clip);

            rect.LowerRightCorner.X -= 1;
            rect.LowerRightCorner.Y -= 1;
            driver->draw2DRectangle(skin->getColor(EGDC_3D_FACE), rect, clip);
        }
    }
    else
    {
        if (States[State])
            States[State]->draw(rect.UpperLeftCorner, clip, video::SColor(0xffffffff));
        else
        {
            driver->draw2DRectangle(skin->getColor(EGDC_3D_HIGH_LIGHT), rect, clip);

            rect.LowerRightCorner.X -= 1;
            rect.LowerRightCorner.Y -= 1;
            driver->draw2DRectangle(skin->getColor(EGDC_3D_DARK_SHADOW), rect, clip);

            rect.UpperLeftCorner.X += 1;
            rect.UpperLeftCorner.Y += 1;
            driver->draw2DRectangle(skin->getColor(EGDC_3D_SHADOW), rect, clip);

            rect.UpperLeftCorner.X += 1;
            rect.UpperLeftCorner.Y += 1;
            driver->draw2DRectangle(skin->getColor(EGDC_3D_FACE), rect, clip);
        }
    }

    if (Text.size())
    {
        rect = AbsoluteRect;
        if (State == EBS_PRESSED)
            rect.UpperLeftCorner.Y += 2;

        if (font)
        {
            video::SColor color = OverrideColor;
            if (!OverrideColorEnabled)
                color = skin->getColor(IsEnabled ? EGDC_BUTTON_TEXT : EGDC_GRAY_TEXT);
            font->draw(Text.c_str(), rect, color, EFHA_CENTER, EFVA_CENTER, clip);
        }
    }

    IGUIElement::draw();
}

//! sends a click to the parent as if the button had been clicked
void CGUIButton::fakeButtonClick()
{
    if (Parent)
    {
        event::SEvent e;
        e.EventType = event::EET_GUI_EVENT;
        e.GUIEvent.Caller = this;
        e.GUIEvent.EventType = EGET_BUTTON_CLICKED;
        Parent->OnEvent(e);
    }
}

//! sets another skin independent font. if this is set to zero, the button uses the font of the skin.
void CGUIButton::setOverrideFont(IGUIFont* font)
{
    if (OverrideFont)
        OverrideFont->drop();

    OverrideFont = font;

    if (OverrideFont)
        OverrideFont->grab();
}

//! sets a text color that replaces the skin's button text colors
void CGUIButton::setOverrideColor(const video::SColor& color)
{
    OverrideColor = color;
    OverrideColorEnabled = true;
}

//! starts the animations of the button states
void CGUIButton::setAnimations(video::ISpritePackage* package, const char* name, bool resize)
{
    const char* BUTTON_ANIMATION_NAMES[EBS_COUNT] = { "Normal", "Highlighted", "Pressed", "Disabled" };

    for (int i = 0; i < EBS_COUNT; ++i)
    {
        if (States[i])
        {
            States[i]->remove();
            States[i] = 0;
        }

        core::CString<char> animation(name);
        animation += BUTTON_ANIMATION_NAMES[i];
        States[i] = package->addNewAnimationState(animation.c_str());
    }

    if (resize && States[EBS_NORMAL])
    {
        core::CDimension2d<int> size = States[EBS_NORMAL]->getFrameSize(0);
        RelativeRect.LowerRightCorner.X = RelativeRect.UpperLeftCorner.X + size.Width;
        RelativeRect.LowerRightCorner.Y = RelativeRect.UpperLeftCorner.Y + size.Height;
        updateAbsolutePosition();
    }
}

//! Sets if the button should behave like a push button.
void CGUIButton::setIsPushButton(bool isPushButton)
{
    IsPushButton = isPushButton;
}

//! Returns if the button is currently pressed
bool CGUIButton::isPressed()
{
    return State == EBS_PRESSED;
}

//! Sets the pressed state of the button
void CGUIButton::setPressed(bool pressed)
{
    if (pressed)
        setNewState(EBS_PRESSED);
    else if (IsEnabled)
        setNewState(EBS_NORMAL);
    else
        setNewState(EBS_DISABLED);
}

//! constructor
CGUITextButton::CGUITextButton(IGUIEnvironment* environment, IGUIElement* parent, const wchar_t* text, int id,
    IGUIFont* font, video::SColor color, const char* sideSprite, const char* topSprite)
    : IGUITextButton(environment, parent, id, core::CRect<int>(0, 0, 10, 10)), Font(font), ButtonText(text),
      Color(color)
{
    Type = EGUIET_TEXT_BUTTON;
    Sprites[ES_SIDE] = 0;
    Sprites[ES_TOP] = 0;
    Sprites[ES_UNUSED] = 0;

    int width = 0;
    int height = 0;

    if (Font)
    {
        TextSize = Font->getDimension(text);
        width = TextSize.Width;
        height = TextSize.Height;
    }

    video::ISpritePackage* package = Environment->getSkin()->getSpritePackage();

    if (sideSprite && package)
    {
        Sprites[ES_SIDE] = package->addNewAnimationState(sideSprite);
        if (Sprites[ES_SIDE])
        {
            core::CDimension2d<int> size = Sprites[ES_SIDE]->getFrameSize(0);
            width += size.Width * 2;
            if (size.Height > height)
                height = size.Height;
        }
    }

    if (topSprite && package)
    {
        Sprites[ES_TOP] = package->addNewAnimationState(topSprite);
        if (Sprites[ES_TOP])
        {
            core::CDimension2d<int> size = Sprites[ES_TOP]->getFrameSize(0);
            height += size.Height;
            if (size.Width > width)
                width = size.Width;
        }
    }

    setRelativePosition(core::CRect<int>(RelativeRect.UpperLeftCorner.X, RelativeRect.UpperLeftCorner.Y,
        RelativeRect.UpperLeftCorner.X + width, RelativeRect.UpperLeftCorner.Y + height));
}

//! destructor
CGUITextButton::~CGUITextButton()
{
    for (int i = 0; i < ES_COUNT; ++i)
        if (Sprites[i])
            Sprites[i]->remove();
}

void CGUITextButton::setTextColor(video::SColor color)
{
    Color = color;
}

//! called if an event happened.
bool CGUITextButton::OnEvent(const event::SEvent& event)
{
    if (isEnabled())
    {
        switch (event.EventType)
        {
        case event::EET_KEY_INPUT_EVENT:
            if ((event.KeyInput.Key == KEY_RETURN || event.KeyInput.Key == KEY_SPACE) &&
                event.KeyInput.Event == event::EKIE_KEY_LEFT_UP)
            {
                Environment->removeFocus(this);

                if (Parent)
                {
                    event::SEvent e;
                    e.EventType = event::EET_GUI_EVENT;
                    e.GUIEvent.Caller = this;
                    e.GUIEvent.EventType = EGET_BUTTON_CLICKED;
                    Parent->OnEvent(e);
                }
                return true;
            }
            break;
        case event::EET_MOUSE_INPUT_EVENT:
            if (event.MouseInput.Event == event::EMIE_LMOUSE_LEFT_UP ||
                event.MouseInput.Event == event::EMIE_RMOUSE_LEFT_UP)
            {
                Environment->removeFocus(this);

                if (Parent)
                {
                    event::SEvent e;
                    e.EventType = event::EET_GUI_EVENT;
                    e.GUIEvent.Caller = this;
                    e.GUIEvent.EventType = event.MouseInput.Event == event::EMIE_LMOUSE_LEFT_UP ?
                        EGET_BUTTON_CLICKED : EGET_TEXT_BUTTON_CLICKED;
                    Parent->OnEvent(e);
                }
                return true;
            }
            break;
        default:
            break;
        }
    }

    return Parent ? Parent->OnEvent(event) : false;
}

//! draws the element and its children
void CGUITextButton::draw()
{
    if (!isVisible())
        return;

    const core::CRect<int>* clip = &AbsoluteClippingRect;

    bool hovered = false;
    if (isEnabled())
        hovered = AbsoluteClippingRect.isPointInside(Environment->getMousePosition());

    int topHeight = 0;
    if (Sprites[ES_TOP])
    {
        core::CDimension2d<int> size = Sprites[ES_TOP]->getFrameSize(0);
        Sprites[ES_TOP]->draw(core::CPosition2d<int>(
            AbsoluteRect.UpperLeftCorner.X + (AbsoluteRect.getWidth() - size.Width) / 2,
            AbsoluteRect.UpperLeftCorner.Y), clip, video::SColor(0xffffffff));
        topHeight = size.Height;
    }

    int textHeight = AbsoluteRect.getHeight() - topHeight;

    if (Font)
    {
        core::CRect<int> rect;
        rect.UpperLeftCorner.X = AbsoluteRect.UpperLeftCorner.X + (AbsoluteRect.getWidth() - TextSize.Width) / 2;
        rect.UpperLeftCorner.Y = AbsoluteRect.UpperLeftCorner.Y + topHeight + (textHeight - TextSize.Height) / 2;
        rect.LowerRightCorner.X = rect.UpperLeftCorner.X + TextSize.Width;
        rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + TextSize.Height;
        Font->draw(ButtonText.c_str(), rect, Color, EFHA_LEFT, EFVA_TOP, clip);
    }

    if (Sprites[ES_SIDE] && hovered)
    {
        core::CDimension2d<int> size = Sprites[ES_SIDE]->getFrameSize(0);
        int y = topHeight + AbsoluteRect.UpperLeftCorner.Y + (textHeight - size.Height) / 2;
        Sprites[ES_SIDE]->draw(core::CPosition2d<int>(AbsoluteRect.UpperLeftCorner.X, y), clip,
            video::SColor(0xffffffff));
        Sprites[ES_SIDE]->draw(core::CPosition2d<int>(AbsoluteRect.LowerRightCorner.X - size.Width, y), clip,
            video::SColor(0xffffffff));
    }

    IGUIElement::draw();
}

//! does nothing
void CGUITextButton::setAnimations(video::ISpritePackage* package, const char* name, bool resize)
{
}

} // end namespace gui
} // end namespace daisy
