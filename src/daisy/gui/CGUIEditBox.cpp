// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIEditBox.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye draws the box with sprite animations when the skin has them, hides passwords, moves the
// focus with Tab, clicks an associated button on Enter, reports text changes and takes pasted text
// as a key event; the shift selection and the copy and cut shortcuts are gone.

#include "CGUIEditBox.h"
#include "daisy/os.h"
#include "ox/core/CStringConversions.h"
#include "ox/gui/IGUIButton.h"
#include "ox/gui/IGUIElementInline.h"
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

//! Clips a rectangle against another one, as Irrlicht's rect::clipAgainst.
static inline void clipAgainst(core::CRect<int>& rect, const core::CRect<int>& other)
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

//! constructor
CGUIEditBox::CGUIEditBox(const wchar_t* text, bool border, IGUIEnvironment* environment, IGUIElement* parent,
    int id, const core::CRect<int>& rectangle, IOSOperator* op)
    : IGUIEditBox(environment, parent, id, rectangle), MouseMarking(false), Border(border),
      OverrideColorEnabled(false), MarkBegin(0), MarkEnd(0), OverrideFont(0), Operator(op), CursorPos(0),
      ScrollPos(0), Max(0), Hidden(false), AssociatedButton(-1)
{
    Type = EGUIET_EDIT_BOX;
    OverrideColor = video::SColor(101, 255, 255, 255);
    Text = text;

    for (int i = 0; i < EEBA_COUNT; ++i)
        Animations[i] = 0;

    int width = rectangle.getWidth();
    int height = rectangle.getHeight();
    AnimationRects[EEBA_LEFT] = core::CRect<int>(0, 0, 3, height);
    AnimationRects[EEBA_RIGHT] = core::CRect<int>(width - 3, 0, width, height);
    AnimationRects[EEBA_BACKGROUND] = core::CRect<int>(3, 0, width - 3, height);

    if (Border && Environment && Environment->getSkin() && Environment->getSkin()->getSpritePackage())
        setAnimations(Environment->getSkin()->getSpritePackage(), "EditBox");

    if (Operator)
        Operator->grab();
}

//! destructor
CGUIEditBox::~CGUIEditBox()
{
    for (int i = 0; i < EEBA_COUNT; ++i)
        if (Animations[i])
            Animations[i]->remove();

    if (OverrideFont)
        OverrideFont->drop();

    if (Operator)
        Operator->drop();
}

//! Moves the right animation along with the right edge.
void CGUIEditBox::setRelativePosition(const core::CRect<int>& position)
{
    IGUIElement::setRelativePosition(position);
    updateAnimationRects();
}

void CGUIEditBox::updateAnimationRects()
{
    if (Animations[EEBA_BACKGROUND])
    {
        int width = RelativeRect.getWidth();
        int offset = width - AnimationRects[EEBA_RIGHT].LowerRightCorner.X;
        AnimationRects[EEBA_RIGHT].LowerRightCorner.X = width;
        AnimationRects[EEBA_RIGHT].UpperLeftCorner.X += offset;
        AnimationRects[EEBA_BACKGROUND].UpperLeftCorner.X = AnimationRects[EEBA_LEFT].LowerRightCorner.X;
        AnimationRects[EEBA_BACKGROUND].LowerRightCorner.X = AnimationRects[EEBA_RIGHT].UpperLeftCorner.X;
    }
}

//! Takes the focus and selects the whole text.
void CGUIEditBox::setFocus()
{
    BlinkStartTime = os::Timer::getTime();
    Environment->setFocus(this);
    MouseMarking = false;
    CursorPos = Text.size();
    MarkBegin = 0;
    MarkEnd = CursorPos;
}

//! Sets another skin independent font.
void CGUIEditBox::setOverrideFont(IGUIFont* font)
{
    if (OverrideFont)
        OverrideFont->drop();

    OverrideFont = font;

    if (OverrideFont)
        OverrideFont->grab();
}

//! Sets another color for the text.
void CGUIEditBox::setOverrideColor(video::SColor color)
{
    OverrideColor = color;
    OverrideColorEnabled = true;
}

//! Sets if the text should use the overide color or the
//! color in the gui skin.
void CGUIEditBox::enableOverrideColor(bool enable)
{
    OverrideColorEnabled = enable;
}

//! The button that Enter clicks.
void CGUIEditBox::setAssociatedButton(int id)
{
    AssociatedButton = id;
}

//! called if an event happened.
bool CGUIEditBox::OnEvent(const event::SEvent& event)
{
    if (isEnabled())
    {
        switch (event.EventType)
        {
        case event::EET_GUI_EVENT:
            if (event.GUIEvent.EventType == EGET_ELEMENT_FOCUS_LOST)
            {
                MouseMarking = false;
                MarkBegin = 0;
                MarkEnd = 0;
                return true;
            }
            break;
        case event::EET_KEY_INPUT_EVENT:
            return processKey(event);
        case event::EET_MOUSE_INPUT_EVENT:
            return processMouse(event);
        default:
            break;
        }
    }

    return Parent ? Parent->OnEvent(event) : false;
}

bool CGUIEditBox::processKey(const event::SEvent& event)
{
    if (event.KeyInput.Event == event::EKIE_KEY_LEFT_UP)
        return false;

    bool textChanged = false;

    if (event.KeyInput.Event == event::EKIE_PASTE)
    {
        // paste from the clipboard
        if (Operator)
        {
            int realmbgn = MarkBegin < MarkEnd ? MarkBegin : MarkEnd;
            int realmend = MarkBegin < MarkEnd ? MarkEnd : MarkBegin;

            // add new character
            char* p = Operator->getTextFromClipboard();
            if (p)
            {
                if (MarkBegin == MarkEnd)
                {
                    // insert text
                    core::CString<wchar_t> s = Text.subString(0, CursorPos);
                    s.append(core::CStringFunctions::ansiToWide(core::CString<char>(p)));
                    s.append(Text.subString(CursorPos, Text.size() - CursorPos));
                    Text = s;
                    s = core::CStringFunctions::ansiToWide(core::CString<char>(p));
                    CursorPos += s.size();
                }
                else
                {
                    // replace text

                    core::CString<wchar_t> s = Text.subString(0, realmbgn);
                    s.append(core::CStringFunctions::ansiToWide(core::CString<char>(p)));
                    s.append(Text.subString(realmend, Text.size() - realmend));
                    Text = s;

                    s = core::CStringFunctions::ansiToWide(core::CString<char>(p));
                    CursorPos = realmbgn + s.size();
                }
            }

            MarkBegin = 0;
            MarkEnd = 0;
        }
        return true;
    }

    if (event.KeyInput.Event == event::EKIE_KEY_PRESSED_DOWN)
    {
        switch (event.KeyInput.Key)
        {
        case KEY_END:
            MarkBegin = 0;
            MarkEnd = 0;
            CursorPos = Text.size();
            BlinkStartTime = os::Timer::getTime();
            break;
        case KEY_HOME:
            MarkBegin = 0;
            MarkEnd = 0;
            CursorPos = 0;
            BlinkStartTime = os::Timer::getTime();
            break;
        case KEY_RETURN:
            {
                event::SEvent e;
                e.EventType = event::EET_GUI_EVENT;
                e.GUIEvent.Caller = this;
                e.GUIEvent.EventType = EGET_EDITBOX_ENTER;
                Parent->OnEvent(e);

                if (AssociatedButton >= 0)
                {
                    IGUIButton* button = (IGUIButton*)Environment->getRootGUIElement()->getElementFromId(
                        AssociatedButton, true);
                    if (button)
                        button->fakeButtonClick();
                }
            }
            return true;
        case KEY_LEFT:
            MarkBegin = 0;
            MarkEnd = 0;

            if (CursorPos > 0)
                CursorPos--;
            BlinkStartTime = os::Timer::getTime();
            break;

        case KEY_RIGHT:
            MarkBegin = 0;
            MarkEnd = 0;

            if (Text.size() > CursorPos)
                CursorPos++;
            BlinkStartTime = os::Timer::getTime();
            break;

        case KEY_BACK:
            if (Text.size() != 0)
            {
                core::CString<wchar_t> s;

                if (MarkBegin != MarkEnd)
                {
                    // delete marked text
                    int realmbgn = MarkBegin < MarkEnd ? MarkBegin : MarkEnd;
                    int realmend = MarkBegin < MarkEnd ? MarkEnd : MarkBegin;

                    s = Text.subString(0, realmbgn);
                    s.append(Text.subString(realmend, Text.size() - realmend));
                    Text = s;

                    CursorPos = realmbgn;
                }
                else
                {
                    // delete text behind cursor
                    s = Text.subString(0, CursorPos - 1);
                    s.append(Text.subString(CursorPos, Text.size() - CursorPos));
                    Text = s;
                    --CursorPos;
                }

                if (CursorPos < 0)
                    CursorPos = 0;
                BlinkStartTime = os::Timer::getTime();
                MarkBegin = 0;
                MarkEnd = 0;
                textChanged = true;
            }
            break;
        case KEY_DELETE:
            if (Text.size() != 0)
            {
                core::CString<wchar_t> s;

                if (MarkBegin != MarkEnd)
                {
                    // delete marked text
                    int realmbgn = MarkBegin < MarkEnd ? MarkBegin : MarkEnd;
                    int realmend = MarkBegin < MarkEnd ? MarkEnd : MarkBegin;

                    s = Text.subString(0, realmbgn);
                    s.append(Text.subString(realmend, Text.size() - realmend));
                    Text = s;

                    CursorPos = realmbgn;
                }
                else
                {
                    // delete text before cursor
                    s = Text.subString(0, CursorPos);
                    s.append(Text.subString(CursorPos + 1, Text.size() - CursorPos - 1));
                    Text = s;
                }

                if (CursorPos > Text.size())
                    CursorPos = Text.size();

                BlinkStartTime = os::Timer::getTime();
                MarkBegin = 0;
                MarkEnd = 0;
                textChanged = true;
            }
            break;
        case KEY_TAB:
            {
                // focus the next edit box of the tab
                IGUIElement* tab = Parent;
                while (tab->getType() == EGUIET_TAB && tab->getParent())
                    tab = tab->getParent();

                for (int id = ID + 1;; ++id)
                {
                    IGUIElement* next = tab->getElementFromId(id, true);
                    if (!next)
                        break;

                    if (next->getType() == EGUIET_EDIT_BOX)
                    {
                        CGUIEditBox* box = (CGUIEditBox*)next;
                        Environment->setFocus(box);
                        box->MarkBegin = 0;
                        box->MarkEnd = box->Text.size();
                        break;
                    }
                }
            }
            break;
        case KEY_ESCAPE:
            return false;
        default:
            break;
        }
    }
    else if (event.KeyInput.Event == event::EKIE_CHARACTER)
    {
        // control characters are not typed
        if (event.KeyInput.Char >= 30)
        {
            if (Text.size() < Max || Max == 0)
            {
                core::CString<wchar_t> s;

                if (MarkBegin != MarkEnd)
                {
                    // replace marked text
                    int realmbgn = MarkBegin < MarkEnd ? MarkBegin : MarkEnd;
                    int realmend = MarkBegin < MarkEnd ? MarkEnd : MarkBegin;

                    s = Text.subString(0, realmbgn);
                    s.append(event.KeyInput.Char);
                    s.append(Text.subString(realmend, Text.size() - realmend));
                    Text = s;
                    CursorPos = realmbgn + 1;
                }
                else
                {
                    // add new character
                    s = Text.subString(0, CursorPos);
                    s.append(event.KeyInput.Char);
                    s.append(Text.subString(CursorPos, Text.size() - CursorPos));
                    Text = s;
                    ++CursorPos;
                }

                BlinkStartTime = os::Timer::getTime();
                MarkBegin = 0;
                MarkEnd = 0;
                textChanged = true;
            }
        }
    }

    // calculate scrollpos

    IGUIFont* font = OverrideFont;
    IGUISkin* skin = Environment->getSkin();
    if (!OverrideFont)
        font = skin->getFont();

    int cursorwidth = font->getDimension(L"_ ").Width;

    int minwidht = cursorwidth * 2;
    if (minwidht >= AbsoluteRect.getWidth())
        minwidht = AbsoluteRect.getWidth() / 2;

    int tries = Text.size() * 2;
    if (tries < 100)
        tries = 100;

    for (int t = 0; t < tries; ++t)
    {
        core::CString<wchar_t> s = Text.subString(0, CursorPos);
        int charcursorpos = font->getDimension(s.c_str()).Width;

        s = Text.subString(0, ScrollPos);
        int charscrollpos = font->getDimension(s.c_str()).Width;

        if ((charcursorpos + cursorwidth - charscrollpos) > AbsoluteRect.getWidth())
            ScrollPos++;
        else if ((charcursorpos + cursorwidth - charscrollpos) < minwidht)
        {
            if (ScrollPos > 0)
                ScrollPos--;
            else
                break;
        }
        else
            break;
    }

    if (textChanged && Parent)
    {
        event::SEvent e;
        e.EventType = event::EET_GUI_EVENT;
        e.GUIEvent.Caller = this;
        e.GUIEvent.EventType = EGET_EDITBOX_CHANGED;
        Parent->OnEvent(e);
    }

    return true;
}

bool CGUIEditBox::processMouse(const event::SEvent& event)
{
    switch (event.MouseInput.Event)
    {
    case event::EMIE_LMOUSE_LEFT_UP:
        if (Environment->hasFocus(this))
        {
            if (AbsoluteRect.isPointInside(core::CPosition2d<int>(event.MouseInput.X, event.MouseInput.Y)) ||
                MouseMarking)
            {
                CursorPos = getCursorPos(event.MouseInput.X);
                if (MouseMarking)
                    MarkEnd = CursorPos;
                MouseMarking = false;
                return true;
            }
        }
        break;
    case event::EMIE_MOUSE_MOVED:
        if (MouseMarking)
        {
            CursorPos = getCursorPos(event.MouseInput.X);
            MarkEnd = CursorPos;
            return true;
        }
        break;
    case event::EMIE_LMOUSE_PRESSED_DOWN:
        if (AbsoluteRect.isPointInside(core::CPosition2d<int>(event.MouseInput.X, event.MouseInput.Y)))
        {
            if (!Environment->hasFocus(this))
            {
                // get focus
                setFocus();
                return true;
            }
            else
            {
                if (!AbsoluteRect.isPointInside(core::CPosition2d<int>(event.MouseInput.X, event.MouseInput.Y)))
                {
                    // remove focus
                    Environment->removeFocus(this);
                    return false;
                }

                // move cursor

                CursorPos = getCursorPos(event.MouseInput.X);

                if (!MouseMarking)
                    MarkBegin = CursorPos;

                MouseMarking = true;
                MarkEnd = CursorPos;
                return true;
            }
        }
        break;
    default:
        break;
    }

    return false;
}

//! draws the element and its children
void CGUIEditBox::draw()
{
    if (!IsVisible)
        return;

    bool focus = Environment->hasFocus(this);

    IGUISkin* skin = Environment->getSkin();
    video::IVideoDriver* driver = Environment->getVideoDriver();

    core::CRect<int> frameRect(AbsoluteRect);

    // draw the border

    if (Border)
    {
        if (Animations[EEBA_BACKGROUND])
        {
            // the background is tiled between the left and right animations
            core::CRect<int> rect = AnimationRects[EEBA_BACKGROUND];
            rect.UpperLeftCorner += frameRect.UpperLeftCorner;
            rect.LowerRightCorner += frameRect.UpperLeftCorner;
            clipAgainst(rect, AbsoluteClippingRect);

            int tileWidth = Animations[EEBA_BACKGROUND]->getFrameSize(0).Width;
            for (int x = rect.UpperLeftCorner.X; x < rect.LowerRightCorner.X; x += tileWidth)
                Animations[EEBA_BACKGROUND]->draw(core::CPosition2d<int>(x, rect.UpperLeftCorner.Y), &rect,
                    video::SColor(0xffffffff));

            rect = AnimationRects[EEBA_LEFT];
            rect.UpperLeftCorner += frameRect.UpperLeftCorner;
            rect.LowerRightCorner += frameRect.UpperLeftCorner;
            clipAgainst(rect, AbsoluteClippingRect);
            Animations[EEBA_LEFT]->draw(rect.UpperLeftCorner, &rect, video::SColor(0xffffffff));

            rect = AnimationRects[EEBA_RIGHT];
            rect.UpperLeftCorner += frameRect.UpperLeftCorner;
            rect.LowerRightCorner += frameRect.UpperLeftCorner;
            clipAgainst(rect, AbsoluteClippingRect);
            Animations[EEBA_RIGHT]->draw(rect.UpperLeftCorner, &rect, video::SColor(0xffffffff));

            frameRect.UpperLeftCorner.X += AnimationRects[EEBA_LEFT].LowerRightCorner.X;
        }
        else
        {
            driver->draw2DRectangle(skin->getColor(EGDC_3D_HIGH_LIGHT), frameRect, &AbsoluteClippingRect);

            frameRect.LowerRightCorner.Y = frameRect.UpperLeftCorner.Y + 1;
            driver->draw2DRectangle(skin->getColor(EGDC_3D_SHADOW), frameRect, &AbsoluteClippingRect);

            frameRect.LowerRightCorner.Y = AbsoluteRect.LowerRightCorner.Y;
            frameRect.LowerRightCorner.X = frameRect.UpperLeftCorner.X + 1;
            driver->draw2DRectangle(skin->getColor(EGDC_3D_SHADOW), frameRect, &AbsoluteClippingRect);

            frameRect = AbsoluteRect;
            frameRect.UpperLeftCorner.X = frameRect.LowerRightCorner.X - 1;
            driver->draw2DRectangle(skin->getColor(EGDC_3D_HIGH_LIGHT), frameRect, &AbsoluteClippingRect);

            frameRect = AbsoluteRect;
            frameRect.UpperLeftCorner.Y = AbsoluteRect.LowerRightCorner.Y - 1;
            frameRect.LowerRightCorner.Y = AbsoluteRect.LowerRightCorner.Y;
            driver->draw2DRectangle(skin->getColor(EGDC_3D_HIGH_LIGHT), frameRect, &AbsoluteClippingRect);

            frameRect = AbsoluteRect;
            frameRect.UpperLeftCorner.X += AnimationRects[EEBA_LEFT].LowerRightCorner.X;
        }
    }

    // draw the text

    IGUIFont* font = OverrideFont;
    if (!OverrideFont)
        font = skin->getFont();

    if (font)
    {
        // hidden text is shown as asterisks
        core::CString<wchar_t> text;
        text.reserve(Text.size());
        if (!Hidden)
            text = Text;
        else
        {
            int length = Text.size();
            for (int i = 0; i < length; ++i)
                text.append(L'*');
        }

        // calculate cursor pos

        core::CString<wchar_t> s = text.subString(0, CursorPos);
        int charcursorpos = font->getDimension(s.c_str()).Width;

        s = text.subString(0, ScrollPos);
        int charscrollpos = font->getDimension(s.c_str()).Width;

        core::CRect<int> rct;

        // draw mark

        if (focus && MarkBegin != MarkEnd)
        {
            rct = frameRect;

            rct.LowerRightCorner.Y -= 2;
            rct.UpperLeftCorner.Y += 2;

            int realmbgn = MarkBegin < MarkEnd ? MarkBegin : MarkEnd;
            int realmend = MarkBegin < MarkEnd ? MarkEnd : MarkBegin;

            s = text.subString(0, realmbgn);
            int mbegin = font->getDimension(s.c_str()).Width;

            s = text.subString(realmbgn, realmend - realmbgn);
            int mend = font->getDimension(s.c_str()).Width;

            rct.UpperLeftCorner.X += mbegin - charscrollpos;
            rct.LowerRightCorner.X = rct.UpperLeftCorner.X + mend;

            driver->draw2DRectangle(skin->getColor(EGDC_HIGH_LIGHT), rct, &AbsoluteClippingRect);
        }

        // draw cursor

        if (focus && (os::Timer::getTime() - BlinkStartTime) % 700 < 350)
        {
            rct = frameRect;
            rct.UpperLeftCorner.X += charcursorpos;
            rct.UpperLeftCorner.X -= charscrollpos;

            font->draw(L"_", rct, OverrideColorEnabled ? OverrideColor : skin->getColor(EGDC_BUTTON_TEXT),
                EFHA_LEFT, EFVA_CENTER, &AbsoluteClippingRect);
        }

        // draw text

        if (text.size())
        {
            rct = frameRect;
            rct.UpperLeftCorner.X -= charscrollpos;

            if (focus && MarkBegin != MarkEnd)
            {
                // marked text

                font->draw(text.c_str(), rct,
                    OverrideColorEnabled ? OverrideColor : skin->getColor(EGDC_BUTTON_TEXT),
                    EFHA_LEFT, EFVA_CENTER, &AbsoluteClippingRect);

                int realmbgn = MarkBegin < MarkEnd ? MarkBegin : MarkEnd;
                int realmend = MarkBegin < MarkEnd ? MarkEnd : MarkBegin;

                s = text.subString(0, realmbgn);
                int mbegin = font->getDimension(s.c_str()).Width;

                s = text.subString(realmbgn, realmend - realmbgn);

                rct.UpperLeftCorner.X += mbegin;

                font->draw(s.c_str(), rct,
                    OverrideColorEnabled ? OverrideColor : skin->getColor(EGDC_HIGH_LIGHT_TEXT),
                    EFHA_LEFT, EFVA_CENTER, &AbsoluteClippingRect);
            }
            else
            {
                // normal text, grayed when disabled
                video::SColor color = OverrideColor;
                if (!OverrideColorEnabled)
                {
                    if (isEnabled())
                        color = skin->getColor(EGDC_BUTTON_TEXT);
                    else
                        color = skin->getColor(EGDC_GRAY_TEXT);
                }
                font->draw(text.c_str(), rct, color, EFHA_LEFT, EFVA_CENTER, &AbsoluteClippingRect);
            }
        }
    }
}

//! Sets the new caption of this element; the cursor goes to the end.
void CGUIEditBox::setText(const wchar_t* text)
{
    Text = text;
    CursorPos = Text.size();
    ScrollPos = 0;
    MarkBegin = 0;
    MarkEnd = 0;
}

//! Sets the maximum amount of characters which may be entered in the box.
//! \param max: Maximum amount of characters. If 0, the character amount is
//! infinity.
void CGUIEditBox::setMax(int max)
{
    Max = max;
    if (Max < 0)
        Max = 0;

    if (Text.size() > Max && Max != 0)
        Text = Text.subString(0, Max);
}

//! Returns maximum amount of characters, previously set by setMax();
int CGUIEditBox::getMax()
{
    return Max;
}

int CGUIEditBox::getCursorPos(int x)
{
    IGUIFont* font = OverrideFont;
    IGUISkin* skin = Environment->getSkin();
    if (!OverrideFont)
        font = skin->getFont();

    // built like the drawn text, but the position is looked up in the real text
    core::CString<wchar_t> text;
    text.reserve(Text.size());
    if (!Hidden)
        text = Text;
    else
    {
        int length = Text.size();
        for (int i = 0; i < length; ++i)
            text.append(L'*');
    }

    core::CString<wchar_t> s = Text.subString(0, ScrollPos);
    int charscrollpos = font->getDimension(s.c_str()).Width;

    int idx = font->getCharacterFromPos(Text.c_str(),
        x - (AbsoluteRect.UpperLeftCorner.X + AnimationRects[EEBA_LEFT].LowerRightCorner.X) + charscrollpos);
    if (idx != -1)
        return idx;

    return Text.size();
}

//! Shows the text as asterisks.
void CGUIEditBox::setHidden(bool hidden)
{
    Hidden = hidden;
}

//! Draws the box with the animations name + "Background", "Left" and "Right" of a package.
void CGUIEditBox::setAnimations(video::ISpritePackage* package, const char* name)
{
    const char* EDITBOX_ANIMATION_NAMES[EEBA_COUNT] = { "Background", "Left", "Right" };

    for (int i = 0; i < EEBA_COUNT; ++i)
    {
        if (Animations[i])
        {
            Animations[i]->remove();
            Animations[i] = 0;
        }

        core::CString<char> animationName = name;
        animationName.append(core::CString<char>(EDITBOX_ANIMATION_NAMES[i]));
        Animations[i] = package->addNewAnimationState(animationName.c_str());

        if (Animations[i])
        {
            core::CDimension2d<int> size = Animations[i]->getFrameSize(0);
            AnimationRects[i] = core::CRect<int>(0, 0, size.Width, size.Height);
        }
    }

    // the box takes the height of the background
    RelativeRect.LowerRightCorner.Y = RelativeRect.UpperLeftCorner.Y +
        AnimationRects[EEBA_BACKGROUND].LowerRightCorner.Y - AnimationRects[EEBA_BACKGROUND].UpperLeftCorner.Y;
    setRelativePosition(RelativeRect);
}

} // end namespace gui
} // end namespace daisy
