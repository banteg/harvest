// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIComboBox.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye adds sprite animations, shows the selected text as the element text, reports selection
// changes to the parent and opens up to ten list rows on the root element.

#include "CGUIComboBox.h"
#include "ox/gui/IGUIElementInline.h"
#include "CGUIListBox.h"
#include "ox/gui/IGUIButton.h"
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

//! The most rows the opened list shows.
static const int MAX_LIST_ROWS = 10;

//! Clips a rectangle against another, as Irrlicht's rect::clipAgainst.
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
CGUIComboBox::CGUIComboBox(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
    : IGUIComboBox(environment, parent, id, rectangle), ListBox(0), Selected(-1)
{
    IGUISkin* skin = Environment->getSkin();
    Type = EGUIET_COMBO_BOX;

    core::CRect<int> r(0, 0, 20, 20);
    ListButton = Environment->addButton(r, this, -1, 0);
    ListButton->setAnimations(skin->getSpritePackage(), "Combobox", true);

    Animations[EA_BACKGROUND] = 0;
    Animations[EA_LEFT] = 0;

    setRelativePosition(rectangle);
    setAnimations(skin->getSpritePackage(), "EditBox");
}

//! destructor
CGUIComboBox::~CGUIComboBox()
{
    for (int i = 0; i < EA_COUNT; ++i)
        if (Animations[i])
            Animations[i]->remove();
}

//! moves the button to the right end and keeps the button's height
void CGUIComboBox::setRelativePosition(const core::CRect<int>& position)
{
    int height = 20;

    if (ListButton)
    {
        height = ListButton->getRelativePosition().getHeight();
        ListButton->moveTo(core::CPosition2d<int>(
            position.getWidth() - ListButton->getRelativePosition().getWidth(), 0));
    }

    core::CRect<int> rect;
    rect.UpperLeftCorner.X = position.UpperLeftCorner.X;
    rect.UpperLeftCorner.Y = position.UpperLeftCorner.Y;
    rect.LowerRightCorner.X = position.LowerRightCorner.X;
    rect.LowerRightCorner.Y = position.UpperLeftCorner.Y + height;
    IGUIElement::setRelativePosition(rect);
}

//! Returns amount of items in box
int CGUIComboBox::getItemCount()
{
    return Items.size();
}

//! returns string of an item. the idx may be a value from 0 to itemCount-1
const wchar_t* CGUIComboBox::getItem(int index)
{
    if (index < 0 || index >= (int)Items.size())
        return 0;

    return Items[index].c_str();
}

//! adds an item and returns the index of it
int CGUIComboBox::addItem(const wchar_t* text)
{
    Items.push_back(core::CString<wchar_t>(text));

    if (Selected == -1)
    {
        Selected = 0;
        setText(Items[0].c_str());
    }

    return Items.size() - 1;
}

//! deletes all items in the combo box
void CGUIComboBox::clear()
{
    Items.clear();
    Selected = -1;
}

//! returns id of selected item. returns -1 if no item is selected.
int CGUIComboBox::getSelected()
{
    return Selected;
}

//! sets the selected item. Set this to -1 if no item should be selected
void CGUIComboBox::setSelected(int index)
{
    if (index < 0 || index >= (int)Items.size())
        return;

    Selected = index;
}

//! called if an event happened.
bool CGUIComboBox::OnEvent(const event::SEvent& event)
{
    switch (event.EventType)
    {
    case event::EET_GUI_EVENT:
        switch (event.GUIEvent.EventType)
        {
        case EGET_BUTTON_CLICKED:
            if (event.GUIEvent.Caller == ListButton)
            {
                openCloseMenu();
                return true;
            }
            break;
        case EGET_LISTBOX_SELECTED_AGAIN:
            if (event.GUIEvent.Caller == ListBox)
                openCloseMenu();
            return true;
        case EGET_LISTBOX_CHANGED:
            if (event.GUIEvent.Caller == ListBox)
            {
                int selected = ListBox->getSelected();
                if (selected >= 0 && selected < (int)Items.size())
                {
                    Selected = selected;
                    setText(Items[selected].c_str());
                }
                openCloseMenu();

                event::SEvent e;
                e.EventType = event::EET_GUI_EVENT;
                e.GUIEvent.Caller = this;
                e.GUIEvent.EventType = EGET_COMBO_BOX_CHANGED;
                Parent->OnEvent(e);
            }
            return true;
        default:
            break;
        }
        break;
    case event::EET_MOUSE_INPUT_EVENT:
        switch (event.MouseInput.Event)
        {
        case event::EMIE_LMOUSE_PRESSED_DOWN:
            {
                if (!ListBox)
                    Environment->removeFocus(this);

                core::CPosition2d<int> p(event.MouseInput.X, event.MouseInput.Y);

                // send to list box
                if (ListBox && ListBox->getAbsolutePosition().isPointInside(p) && ListBox->OnEvent(event))
                    return true;

                // check if it is outside
                if (!AbsoluteRect.isPointInside(p))
                {
                    Environment->removeFocus(this);
                    return false;
                }
                return true;
            }
        case event::EMIE_LMOUSE_LEFT_UP:
            {
                core::CPosition2d<int> p(event.MouseInput.X, event.MouseInput.Y);

                // send to list box
                if (ListBox && ListBox->getAbsolutePosition().isPointInside(p) && ListBox->OnEvent(event))
                    return true;
                else
                    openCloseMenu();

                if (!AbsoluteRect.isPointInside(p))
                {
                    Environment->removeFocus(this);
                    return false;
                }
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

//! draws the element and its children
void CGUIComboBox::draw()
{
    if (!IsVisible)
        return;

    IGUISkin* skin = Environment->getSkin();

    core::CRect<int> frameRect(AbsoluteRect);

    if (!Animations[EA_BACKGROUND])
    {
        video::IVideoDriver* driver = Environment->getVideoDriver();

        // draw the border

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
    }
    else
    {
        // the left end
        core::CRect<int> rect(AnimationRects[EA_LEFT]);
        rect.UpperLeftCorner += frameRect.UpperLeftCorner;
        rect.LowerRightCorner += frameRect.UpperLeftCorner;
        clipAgainst(rect, AbsoluteClippingRect);
        Animations[EA_LEFT]->draw(rect.UpperLeftCorner, &rect, video::SColor(0xffffffff));

        // the background, repeated up to the button
        rect = AnimationRects[EA_BACKGROUND];
        rect.UpperLeftCorner += frameRect.UpperLeftCorner;
        rect.LowerRightCorner += frameRect.UpperLeftCorner;
        rect.UpperLeftCorner.X += AnimationRects[EA_LEFT].getWidth();
        rect.LowerRightCorner.X = ListButton->getAbsolutePosition().UpperLeftCorner.X;
        clipAgainst(rect, AbsoluteClippingRect);

        int width = Animations[EA_BACKGROUND]->getFrameSize(0).Width;
        for (int x = rect.UpperLeftCorner.X; x < rect.LowerRightCorner.X; x += width)
            Animations[EA_BACKGROUND]->draw(core::CPosition2d<int>(x, rect.UpperLeftCorner.Y), &rect,
                video::SColor(0xffffffff));
    }

    // Draw text

    if (Selected != -1)
    {
        frameRect = AbsoluteRect;
        frameRect.UpperLeftCorner.X += 2;

        skin->getFont()->draw(Items[Selected].c_str(), frameRect, skin->getColor(EGDC_BUTTON_TEXT), EFHA_LEFT,
            EFVA_CENTER, &AbsoluteClippingRect);
    }

    // draw buttons
    IGUIElement::draw();
}

void CGUIComboBox::openCloseMenu()
{
    if (ListBox)
    {
        // close list box
        ListBox->remove();
        ListBox = 0;
    }
    else
    {
        if (Parent)
            Parent->bringToFront(this);

        IGUISkin* skin = Environment->getSkin();
        int h = Items.size();

        if (h > MAX_LIST_ROWS)
            h = MAX_LIST_ROWS;
        if (h == 0)
            h = 1;

        h *= (skin->getFont()->getDimension(L"A").Height + 1);

        // open list box on the root element, below the box
        core::CRect<int> r(AbsoluteRect);
        r.UpperLeftCorner.Y = AbsoluteRect.LowerRightCorner.Y;
        r.LowerRightCorner.Y = AbsoluteRect.LowerRightCorner.Y + AbsoluteRect.getHeight() + h;

        CGUIListBox* listBox = new CGUIListBox(Environment, Environment->getRootGUIElement(), -1, r, false, true,
            true);
        listBox->drop();

        for (int i = 0; i < (int)Items.size(); ++i)
            listBox->addTextItem(Items[i].c_str(), 0, video::SColor(0xffffffff), false, false);

        listBox->sortItems(false);
        listBox->setSelected(Selected);
        listBox->setOverrideActionParent(this);
        listBox->setSelectable(true);
        ListBox = listBox;

        // set focus
        Environment->setFocus(ListBox);
    }
}

//! starts the animations of the box parts
void CGUIComboBox::setAnimations(video::ISpritePackage* package, const char* name)
{
    const char* COMBOBOX_ANIMATION_NAMES[EA_COUNT] = { "Background", "Left" };

    for (int i = 0; i < EA_COUNT; ++i)
    {
        if (Animations[i])
        {
            Animations[i]->remove();
            Animations[i] = 0;
        }

        core::CString<char> animation(name);
        animation += COMBOBOX_ANIMATION_NAMES[i];
        Animations[i] = package->addNewAnimationState(animation.c_str());

        if (Animations[i])
        {
            core::CDimension2d<int> size = Animations[i]->getFrameSize(0);
            AnimationRects[i] = core::CRect<int>(0, 0, size.Width, size.Height);
        }
    }
}

} // end namespace gui
} // end namespace daisy
