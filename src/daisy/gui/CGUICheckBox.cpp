// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUICheckBox.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye adds sprite animations, a highlighted state and the text font and color.

#include "CGUICheckBox.h"
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

//! The built-in font's check mark, from Irrlicht's GUIIcons.h.
static const wchar_t GUI_ICON_CHECK_BOX_CHECKED[] = { 265, 0 };

//! constructor
CGUICheckBox::CGUICheckBox(bool checked, IGUIEnvironment* environment, IGUIElement* parent, int id,
    core::CRect<int> rectangle)
    : IGUICheckBox(environment, parent, id, rectangle), Pressed(false), Checked(checked), Highlighted(false)
{
    Type = EGUIET_CHECK_BOX;

    for (int i = 0; i < EA_COUNT; ++i)
        Animations[i] = 0;

    video::ISpritePackage* package = Environment->getSkin()->getSpritePackage();
    if (package)
        setAnimations(package, "Checkbox");

    TextColor = Environment->getSkin()->getColor(EGDC_BUTTON_TEXT);
    TextFont = Environment->getSkin()->getFont();
}

//! destructor
CGUICheckBox::~CGUICheckBox()
{
    for (int i = 0; i < EA_COUNT; ++i)
        if (Animations[i])
            Animations[i]->remove();
}

//! called if an event happened.
bool CGUICheckBox::OnEvent(const event::SEvent& event)
{
    if (isEnabled() && isVisible())
    {
        switch (event.EventType)
        {
        case event::EET_GUI_EVENT:
            if (event.GUIEvent.EventType == EGET_ELEMENT_FOCUS_LOST ||
                event.GUIEvent.EventType == EGET_ELEMENT_LEFT)
            {
                Pressed = false;
                Highlighted = false;
                return true;
            }
            else if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED)
                Highlighted = true;
            break;
        case event::EET_MOUSE_INPUT_EVENT:
            if (event.MouseInput.Event == event::EMIE_LMOUSE_PRESSED_DOWN)
            {
                Pressed = true;
                Environment->setFocus(this);
                return true;
            }
            else if (event.MouseInput.Event == event::EMIE_LMOUSE_LEFT_UP)
            {
                bool wasPressed = Pressed;
                Environment->removeFocus(this);
                Pressed = false;

                if (wasPressed && Parent)
                {
                    event::SEvent e;
                    e.EventType = event::EET_GUI_EVENT;
                    e.GUIEvent.Caller = this;
                    Checked = !Checked;
                    e.GUIEvent.EventType = EGET_CHECKBOX_TOGGLED;
                    Parent->OnEvent(e);
                    return true;
                }
            }
            break;
        default:
            break;
        }
    }

    return Parent ? Parent->OnEvent(event) : false;
}

//! draws the element and its children
void CGUICheckBox::draw()
{
    if (!IsVisible)
        return;

    IGUISkin* skin = Environment->getSkin();
    video::IVideoDriver* driver = Environment->getVideoDriver();

    core::CRect<int> rect = AbsoluteRect;

    int height = skin->getSize(EGDS_CHECK_BOX_WIDTH);

    core::CRect<int> checkRect(AbsoluteRect.UpperLeftCorner.X,
        ((AbsoluteRect.getHeight() - height) / 2) + AbsoluteRect.UpperLeftCorner.Y, 0, 0);

    int animation = isEnabled() ? EA_NORMAL : EA_DISABLED;
    if (Highlighted)
        animation = EA_HIGHLIGHTED;
    if (isChecked())
        animation += EA_CHECKED_NORMAL;

    if (Animations[animation])
    {
        core::CDimension2d<int> size = Animations[animation]->getFrameSize(0);
        height = rect.getHeight();

        if (Text.size())
            Animations[animation]->draw(core::CPosition2d<int>(rect.UpperLeftCorner.X + (height + 10 - size.Width) / 2,
                rect.UpperLeftCorner.Y + (height - size.Height) / 2), &AbsoluteClippingRect, video::SColor(0xffffffff));
        else
            Animations[animation]->draw(rect.UpperLeftCorner, &AbsoluteClippingRect, video::SColor(0xffffffff));
    }
    else
    {
        checkRect.LowerRightCorner.X = checkRect.UpperLeftCorner.X + height;
        checkRect.LowerRightCorner.Y = checkRect.UpperLeftCorner.Y + height;

        driver->draw2DRectangle(skin->getColor(EGDC_3D_HIGH_LIGHT), checkRect, &AbsoluteClippingRect);

        checkRect.LowerRightCorner.X -= 1;
        checkRect.LowerRightCorner.Y -= 1;
        driver->draw2DRectangle(skin->getColor(EGDC_3D_SHADOW), checkRect, &AbsoluteClippingRect);

        checkRect.UpperLeftCorner.X += 1;
        checkRect.UpperLeftCorner.Y += 1;
        driver->draw2DRectangle(skin->getColor(EGDC_3D_LIGHT), checkRect, &AbsoluteClippingRect);

        checkRect.LowerRightCorner.X -= 1;
        checkRect.LowerRightCorner.Y -= 1;
        driver->draw2DRectangle(skin->getColor(EGDC_3D_DARK_SHADOW), checkRect, &AbsoluteClippingRect);

        checkRect.UpperLeftCorner.X += 1;
        checkRect.UpperLeftCorner.Y += 1;
        driver->draw2DRectangle(skin->getColor(Pressed ? EGDC_3D_FACE : EGDC_ACTIVE_CAPTION), checkRect,
            &AbsoluteClippingRect);

        if (Checked && Environment->getBuiltInFont())
            Environment->getBuiltInFont()->draw(GUI_ICON_CHECK_BOX_CHECKED, checkRect,
                skin->getColor(EGDC_BUTTON_TEXT), EFHA_CENTER, EFVA_CENTER, &checkRect);
    }

    if (Text.size())
    {
        checkRect = AbsoluteRect;
        checkRect.UpperLeftCorner.X += height + 10;

        if (TextFont)
            TextFont->draw(Text.c_str(), checkRect, TextColor, EFHA_LEFT, EFVA_CENTER, &AbsoluteClippingRect);
    }

    IGUIElement::draw();
}

//! set if box is checked
void CGUICheckBox::setChecked(bool checked)
{
    Checked = checked;
}

//! returns if box is checked
bool CGUICheckBox::isChecked()
{
    return Checked;
}

//! starts the animations of the box states
void CGUICheckBox::setAnimations(video::ISpritePackage* package, const char* name)
{
    const char* CHECKBOX_ANIMATION_NAMES[EA_COUNT] = { "Normal", "Highlighted", "Disabled", "CheckedNormal",
        "CheckedHighlighted", "CheckedDisabled" };

    int height = 0;
    int width = 0;

    for (int i = 0; i < EA_COUNT; ++i)
    {
        if (Animations[i])
        {
            Animations[i]->remove();
            Animations[i] = 0;
        }

        core::CString<char> animation(name);
        animation += CHECKBOX_ANIMATION_NAMES[i];
        Animations[i] = package->addNewAnimationState(animation.c_str());

        if (Animations[i])
        {
            if (width < Animations[i]->getFrameSize(0).Width)
                width = Animations[i]->getFrameSize(0).Width;
            if (height < Animations[i]->getFrameSize(0).Height)
                height = Animations[i]->getFrameSize(0).Height;
        }
    }

    if (Animations[EA_NORMAL])
    {
        int elementWidth = RelativeRect.getWidth();
        if (elementWidth < width)
            elementWidth = width;

        RelativeRect.LowerRightCorner.X = RelativeRect.UpperLeftCorner.X + elementWidth;
        RelativeRect.LowerRightCorner.Y = RelativeRect.UpperLeftCorner.Y + height;
        updateAbsolutePosition();
    }
}

void CGUICheckBox::setTextColor(video::SColor color)
{
    TextColor = color;
}

void CGUICheckBox::setTextFont(IGUIFont* font)
{
    TextFont = font;
}

//! widens the element to fit the box and the text
void CGUICheckBox::updateWidth()
{
    if (TextFont && Text.size())
    {
        core::CRect<int> rect = RelativeRect;
        core::CDimension2d<int> dimension = TextFont->getDimension(Text.c_str());
        int width = dimension.Width + rect.getHeight() + 10;

        if (rect.getWidth() < width)
        {
            rect.LowerRightCorner.X = rect.UpperLeftCorner.X + width;
            setRelativePosition(rect);
        }
    }
}

} // end namespace gui
} // end namespace daisy
