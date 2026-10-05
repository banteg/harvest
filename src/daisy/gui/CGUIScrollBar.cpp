// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIScrollBar.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye draws the bar with sprite animations of the skin's package, sizes the thumb from them, pages
// with clicks beside the thumb and highlights the thumb under the mouse.

#include "CGUIScrollBar.h"
#include "CGUIButton.h"
#include "GUIIcons.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUISkin.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

//! constructor
CGUIScrollBar::CGUIScrollBar(bool horizontal, ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent,
    int id, ox::core::CRect<int> rectangle, bool noclip)
    : IGUIScrollBar(environment, parent, id, rectangle), UpButton(0), DownButton(0), CurrentThumb(0),
      OriginalRect(rectangle), Dragging(false), Horizontal(horizontal), NoClip(noclip), Pos(0), DrawPos(0),
      DrawHeight(0), Max(100), SmallStep(10), LargeStep(30), ListBoxParent(0)
{
    Type = ox::gui::EGUIET_SCROLL_BAR;

    for (int i = 0; i < ESBA_COUNT; ++i)
        Animations[i] = 0;

    int size;

    if (Horizontal)
    {
        size = RelativeRect.getHeight();
        UpButton = new CGUIButton(Environment, this, -1, ox::core::CRect<int>(0, 0, size, size), NoClip);
        UpButton->setText(GUI_ICON_CURSOR_LEFT);
        UpButton->drop();
        DownButton = new CGUIButton(Environment, this, -1,
            ox::core::CRect<int>(RelativeRect.getWidth() - size, 0, RelativeRect.getWidth(), size), NoClip);
        DownButton->setText(GUI_ICON_CURSOR_RIGHT);
        DownButton->drop();
    }
    else
    {
        size = RelativeRect.getWidth();
        UpButton = new CGUIButton(Environment, this, -1, ox::core::CRect<int>(0, 0, size, size), NoClip);
        UpButton->setText(GUI_ICON_CURSOR_UP);
        UpButton->drop();
        DownButton = new CGUIButton(Environment, this, -1,
            ox::core::CRect<int>(0, RelativeRect.getHeight() - size, size, RelativeRect.getHeight()), NoClip);
        DownButton->setText(GUI_ICON_CURSOR_DOWN);
        DownButton->drop();
    }

    if (UpButton)
    {
        UpButton->setOverrideFont(Environment->getBuiltInFont());
        UpButton->grab();
    }

    if (DownButton)
    {
        DownButton->setOverrideFont(Environment->getBuiltInFont());
        DownButton->grab();
    }

    if (!Environment->getSkin()->getSpritePackage())
    {
        // without sprites the thumb is as big as the buttons
        AnimationRects[ESBA_THUMB_NORMAL] = ox::core::CRect<int>(0, 0, size, size);
        AnimationRects[ESBA_BACKGROUND] = ox::core::CRect<int>(0, size, size, RelativeRect.getHeight() - size);
        if (Horizontal)
            AnimationRects[ESBA_BACKGROUND] = ox::core::CRect<int>(size, 0, RelativeRect.getWidth() - size, size);
    }
    else if (Horizontal)
        setAnimations(Environment->getSkin()->getSpritePackage(), "HorScroll");
    else
        setAnimations(Environment->getSkin()->getSpritePackage(), "VerScroll");

    setPos(0);
}

//! destructor
CGUIScrollBar::~CGUIScrollBar()
{
    if (UpButton)
        UpButton->drop();

    if (DownButton)
        DownButton->drop();

    for (int i = 0; i < ESBA_COUNT; ++i)
        if (Animations[i])
            Animations[i]->remove();
}

void CGUIScrollBar::setRelativePosition(const ox::core::CRect<int>& position)
{
    IGUIElement::setRelativePosition(position);

    if (Horizontal)
    {
        if (DownButton)
            DownButton->moveTo(ox::core::CPosition2d<int>(
                position.getWidth() - DownButton->getRelativePosition().getWidth(), 0));
    }
    else if (DownButton)
        DownButton->moveTo(ox::core::CPosition2d<int>(
            0, position.getHeight() - DownButton->getRelativePosition().getHeight()));

    updateBackgroundSize();
}

void CGUIScrollBar::updateBackgroundSize()
{
    if (Horizontal)
    {
        ox::core::CRect<int> up = UpButton->getRelativePosition();
        ox::core::CRect<int> down = DownButton->getRelativePosition();
        RelativeRect.UpperLeftCorner.Y = RelativeRect.LowerRightCorner.Y - down.getHeight();
        AnimationRects[ESBA_BACKGROUND] = ox::core::CRect<int>(up.getWidth(), 0, down.UpperLeftCorner.X,
            RelativeRect.LowerRightCorner.Y);
    }
    else
    {
        ox::core::CRect<int> up = UpButton->getRelativePosition();
        ox::core::CRect<int> down = DownButton->getRelativePosition();
        RelativeRect.UpperLeftCorner.X = RelativeRect.LowerRightCorner.X - down.getWidth();
        AnimationRects[ESBA_BACKGROUND] = ox::core::CRect<int>(0, up.getHeight(), RelativeRect.LowerRightCorner.X,
            down.UpperLeftCorner.Y);
    }
}

//! called if an event happened.
bool CGUIScrollBar::OnEvent(const ox::event::SEvent& event)
{
    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
        if (event.GUIEvent.EventType == ox::gui::EGET_BUTTON_CLICKED)
        {
            if (event.GUIEvent.Caller == UpButton)
                setPos(Pos - SmallStep);
            else if (event.GUIEvent.Caller == DownButton)
                setPos(Pos + SmallStep);

            setNewThumbState(0);

            ox::event::SEvent newEvent;
            newEvent.EventType = ox::event::EET_GUI_EVENT;
            newEvent.GUIEvent.Caller = this;
            newEvent.GUIEvent.EventType = ox::gui::EGET_SCROLL_BAR_CHANGED;
            Parent->OnEvent(newEvent);
            return true;
        }
        else if (event.GUIEvent.EventType == ox::gui::EGET_ELEMENT_FOCUS_LOST)
        {
            Dragging = false;
            setNewThumbState(0);
            return true;
        }
        break;
    case ox::event::EET_MOUSE_INPUT_EVENT:
        switch (event.MouseInput.Event)
        {
        case ox::event::EMIE_MOUSE_WHEEL:
        {
            setPos(getPos() + (int)event.MouseInput.ScrollY * -10);
            setNewThumbState(0);

            ox::event::SEvent newEvent;
            newEvent.EventType = ox::event::EET_GUI_EVENT;
            newEvent.GUIEvent.Caller = this;
            newEvent.GUIEvent.EventType = ox::gui::EGET_SCROLL_BAR_CHANGED;
            Parent->OnEvent(newEvent);
            return false;
        }
        case ox::event::EMIE_LMOUSE_PRESSED_DOWN:
            if (Horizontal)
            {
                int offset = event.MouseInput.X - AbsoluteRect.UpperLeftCorner.X - DrawPos;
                if (offset < 0)
                {
                    setPos(getPos() - LargeStep);
                    setNewThumbState(0);
                    if (Parent)
                    {
                        ox::event::SEvent newEvent;
                        newEvent.EventType = ox::event::EET_GUI_EVENT;
                        newEvent.GUIEvent.Caller = this;
                        newEvent.GUIEvent.EventType = ox::gui::EGET_SCROLL_BAR_CHANGED;
                        Parent->OnEvent(newEvent);
                    }
                }
                else if (offset > DrawHeight)
                {
                    setPos(getPos() + LargeStep);
                    setNewThumbState(0);
                    if (Parent)
                    {
                        ox::event::SEvent newEvent;
                        newEvent.EventType = ox::event::EET_GUI_EVENT;
                        newEvent.GUIEvent.Caller = this;
                        newEvent.GUIEvent.EventType = ox::gui::EGET_SCROLL_BAR_CHANGED;
                        Parent->OnEvent(newEvent);
                    }
                }
                else
                {
                    Dragging = true;
                    setNewThumbState(2);
                }
            }
            else
            {
                // the vertical bar does not report its page steps
                int offset = event.MouseInput.Y - AbsoluteRect.UpperLeftCorner.Y - DrawPos;
                if (offset < 0)
                {
                    setPos(getPos() - LargeStep);
                    setNewThumbState(0);
                }
                else if (offset > DrawHeight)
                {
                    setPos(getPos() + LargeStep);
                    setNewThumbState(0);
                }
                else
                {
                    Dragging = true;
                    setNewThumbState(2);
                }
            }
            Environment->setFocus(this);
            return true;
        case ox::event::EMIE_LMOUSE_LEFT_UP:
            setNewThumbState(Dragging ? 1 : 0);
            Dragging = false;
            Environment->removeFocus(this);
            return true;
        case ox::event::EMIE_MOUSE_MOVED:
            if (Dragging)
            {
                int oldPos = Pos;
                setPosFromMousePos(event.MouseInput.X, event.MouseInput.Y);
                setNewThumbState(2);
                if (Pos != oldPos && Parent)
                {
                    ox::event::SEvent newEvent;
                    newEvent.EventType = ox::event::EET_GUI_EVENT;
                    newEvent.GUIEvent.Caller = this;
                    newEvent.GUIEvent.EventType = ox::gui::EGET_SCROLL_BAR_CHANGED;
                    Parent->OnEvent(newEvent);
                }
                return true;
            }
            else
            {
                // highlight the thumb under the mouse
                int offset;
                if (Horizontal)
                    offset = event.MouseInput.X - AbsoluteRect.UpperLeftCorner.X;
                else
                    offset = event.MouseInput.Y - AbsoluteRect.UpperLeftCorner.Y;
                offset -= DrawPos;
                setNewThumbState(offset < 0 || offset >= DrawHeight ? 0 : 1);
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return Parent ? Parent->OnEvent(event) : false;
}

void CGUIScrollBar::setNewThumbState(int state)
{
    ox::video::ISpriteAnimationState* thumb = Animations[ESBA_THUMB_NORMAL + state];
    if (!isEnabled())
        thumb = Animations[ESBA_THUMB_DISABLED];
    if (!thumb)
        thumb = Animations[ESBA_THUMB_NORMAL];

    if (thumb && CurrentThumb != thumb)
    {
        CurrentThumb = thumb;
        thumb->reset();
    }
}

void CGUIScrollBar::setPosFromMousePos(int x, int y)
{
    if (Horizontal)
    {
        float f = (float)(AnimationRects[ESBA_BACKGROUND].getWidth() - DrawHeight) / (float)Max;
        setPos((int)((float)(x - AbsoluteRect.UpperLeftCorner.X - DrawHeight - DrawHeight / 2) / f));
    }
    else
    {
        float f = (float)(AnimationRects[ESBA_BACKGROUND].getHeight() - DrawHeight) / (float)Max;
        setPos((int)((float)(y - AbsoluteRect.UpperLeftCorner.Y - DrawHeight - DrawHeight / 2) / f));
    }
}

//! Clips the rectangle against another one, as Irrlicht's rect::clipAgainst does.
static inline void clipAgainst(ox::core::CRect<int>& rect, const ox::core::CRect<int>& other)
{
    if (other.LowerRightCorner.X < rect.LowerRightCorner.X)
        rect.LowerRightCorner.X = other.LowerRightCorner.X;
    if (other.LowerRightCorner.Y < rect.LowerRightCorner.Y)
        rect.LowerRightCorner.Y = other.LowerRightCorner.Y;
    if (other.UpperLeftCorner.X > rect.UpperLeftCorner.X)
        rect.UpperLeftCorner.X = other.UpperLeftCorner.X;
    if (other.UpperLeftCorner.Y > rect.UpperLeftCorner.Y)
        rect.UpperLeftCorner.Y = other.UpperLeftCorner.Y;
}

//! draws the element and its children
void CGUIScrollBar::draw()
{
    if (!IsVisible)
        return;

    ox::gui::IGUISkin* skin = Environment->getSkin();
    ox::video::IVideoDriver* driver = Environment->getVideoDriver();

    ox::core::CRect<int> rect = AbsoluteRect;
    ox::core::CRect<int>* clip = 0;
    if (!NoClip)
        clip = &AbsoluteClippingRect;

    // draws the background
    if (Animations[ESBA_BACKGROUND])
    {
        ox::core::CRect<int> background(AnimationRects[ESBA_BACKGROUND].UpperLeftCorner + AbsoluteRect.UpperLeftCorner,
            AnimationRects[ESBA_BACKGROUND].LowerRightCorner + AbsoluteRect.UpperLeftCorner);
        if (clip)
            clipAgainst(background, *clip);

        if (Horizontal)
        {
            ox::core::CDimension2d<int> size = Animations[ESBA_BACKGROUND]->getFrameSize(0);
            for (int x = background.UpperLeftCorner.X; x < background.LowerRightCorner.X; x += size.Width)
                Animations[ESBA_BACKGROUND]->draw(ox::core::CPosition2d<int>(x, background.UpperLeftCorner.Y),
                    &background, ox::video::SColor(0xffffffff));
        }
        else
        {
            ox::core::CDimension2d<int> size = Animations[ESBA_BACKGROUND]->getFrameSize(0);
            for (int y = background.UpperLeftCorner.Y; y < background.LowerRightCorner.Y; y += size.Height)
                Animations[ESBA_BACKGROUND]->draw(ox::core::CPosition2d<int>(background.UpperLeftCorner.X, y),
                    &background, ox::video::SColor(0xffffffff));
        }
    }
    else
        driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_SCROLLBAR), rect, clip);

    if (Max != 0)
    {
        // draw thumb
        if (CurrentThumb)
        {
            if (Horizontal)
                CurrentThumb->draw(ox::core::CPosition2d<int>(AbsoluteRect.UpperLeftCorner.X + DrawPos,
                    AbsoluteRect.UpperLeftCorner.Y), clip, ox::video::SColor(0xffffffff));
            else
                CurrentThumb->draw(ox::core::CPosition2d<int>(AbsoluteRect.UpperLeftCorner.X,
                    AbsoluteRect.UpperLeftCorner.Y + DrawPos), clip, ox::video::SColor(0xffffffff));
        }
        else
        {
            if (Horizontal)
            {
                rect.UpperLeftCorner.X = AbsoluteRect.UpperLeftCorner.X + DrawPos + RelativeRect.getHeight() - DrawHeight / 2;
                rect.LowerRightCorner.X = rect.UpperLeftCorner.X + DrawHeight;
            }
            else
            {
                rect.UpperLeftCorner.Y = AbsoluteRect.UpperLeftCorner.Y + DrawPos + RelativeRect.getWidth() - DrawHeight / 2;
                rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + DrawHeight;
            }

            driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_DARK_SHADOW), rect, clip);
            rect.LowerRightCorner.X -= 1;
            rect.LowerRightCorner.Y -= 1;
            driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_HIGH_LIGHT), rect, clip);
            rect.UpperLeftCorner.X += 1;
            rect.UpperLeftCorner.Y += 1;
            driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_SHADOW), rect, clip);
            rect.LowerRightCorner.X -= 1;
            rect.LowerRightCorner.Y -= 1;
            driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_FACE), rect, clip);
        }
    }

    // draw buttons
    IGUIElement::draw();
}

//! sets the position of the scrollbar
void CGUIScrollBar::setPos(int pos)
{
    Pos = pos;
    if (Pos < 0)
        Pos = 0;
    if (Pos > Max)
        Pos = Max;

    if (Horizontal)
    {
        DrawHeight = AnimationRects[ESBA_THUMB_NORMAL].getWidth();
        float f = (float)(AnimationRects[ESBA_BACKGROUND].getWidth() - DrawHeight) / (float)Max;
        DrawPos = (int)((Pos * f) + (float)DrawHeight);
    }
    else
    {
        DrawHeight = AnimationRects[ESBA_THUMB_NORMAL].getHeight();
        float f = (float)(AnimationRects[ESBA_BACKGROUND].getHeight() - DrawHeight) / (float)Max;
        DrawPos = (int)((Pos * f) + (float)DrawHeight);
    }
}

//! sets the maximum value of the scrollbar. must be > 0
void CGUIScrollBar::setMax(int max)
{
    if (max > 0)
        Max = max;
    else
        Max = 0;

    bool enable = (Max != 0);
    UpButton->setEnabled(enable);
    DownButton->setEnabled(enable);

    int smallStep = Max / 50;
    SmallStep = smallStep > 0 ? smallStep : 1;
    LargeStep = Max / 5;
}

void CGUIScrollBar::setStepSizes(int smallStep, int largeStep)
{
    SmallStep = smallStep > 0 ? smallStep : 1;
    LargeStep = largeStep > 0 ? largeStep : 1;
}

int CGUIScrollBar::getMax()
{
    return Max;
}

bool CGUIScrollBar::isDragging()
{
    return Dragging;
}

//! gets the current position of the scrollbar
int CGUIScrollBar::getPos()
{
    return Pos;
}

void CGUIScrollBar::setAnimations(ox::video::ISpritePackage* package, const char* name)
{
    const char* const SCROLLBAR_ANIMATION_NAMES[ESBA_COUNT] =
        { "Background", "ThumbNormal", "ThumbHighlighted", "ThumbPressed", "ThumbDisabled" };

    for (int i = 0; i < ESBA_COUNT; ++i)
    {
        if (Animations[i])
        {
            Animations[i]->remove();
            Animations[i] = 0;
        }

        ox::core::CString<char> animation = name;
        animation.append(SCROLLBAR_ANIMATION_NAMES[i]);
        Animations[i] = package->addNewAnimationState(animation.c_str());

        if (Animations[i])
        {
            ox::core::CDimension2d<int> size = Animations[i]->getFrameSize(0);
            AnimationRects[i] = ox::core::CRect<int>(0, 0, size.Width, size.Height);
        }
    }

    // packages without thumb states have one thumb animation
    if (!Animations[ESBA_THUMB_NORMAL])
    {
        ox::core::CString<char> animation = name;
        animation.append("Thumb");
        Animations[ESBA_THUMB_NORMAL] = package->addNewAnimationState(animation.c_str());

        if (Animations[ESBA_THUMB_NORMAL])
        {
            ox::core::CDimension2d<int> size = Animations[ESBA_THUMB_NORMAL]->getFrameSize(0);
            AnimationRects[ESBA_THUMB_NORMAL] = ox::core::CRect<int>(0, 0, size.Width, size.Height);
        }
    }

    ox::core::CString<char> animation = name;
    animation.append(Horizontal ? "Left" : "Up");
    UpButton->setAnimations(package, animation.c_str(), true);
    UpButton->setText(L"");

    animation = name;
    animation.append(Horizontal ? "Right" : "Down");
    DownButton->setAnimations(package, animation.c_str(), true);
    DownButton->setText(L"");

    RelativeRect = OriginalRect;
    updateBackgroundSize();
    setNewThumbState(0);
}

void CGUIScrollBar::setListBoxParent(CGUIListBox* listBox)
{
    ListBoxParent = listBox;
}

} // end namespace gui
} // end namespace daisy
