// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIContextMenu.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye keeps the items in a std::vector and draws the children after the menu.

#include "CGUIContextMenu.h"
#include "GUIIcons.h"
#include "ox/gui/IGUIElementInline.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/gui/IGUISkin.h"
#include "ox/video/IVideoDriver.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

using namespace ox;
using namespace ox::gui;

//! constructor
CGUIContextMenu::CGUIContextMenu(IGUIEnvironment* environment, IGUIElement* parent, int id,
    core::CRect<int> rectangle, bool getFocus)
    : IGUIContextMenu(environment, parent, id, rectangle), HighLighted(-1)
{
    Pos = rectangle.UpperLeftCorner;
    recalculateSize();

    if (getFocus)
        Environment->setFocus(this);
}

//! destructor
CGUIContextMenu::~CGUIContextMenu()
{
    for (int i = 0; i < (int)Items.size(); ++i)
        if (Items[i].SubMenu)
            Items[i].SubMenu->drop();
}

//! Returns amount of menu items
int CGUIContextMenu::getItemCount() const
{
    return Items.size();
}

//! Adds a menu item.
int CGUIContextMenu::addItem(wchar_t* text, int id, bool enabled, bool hasSubMenu)
{
    SItem s;
    s.Enabled = enabled;
    s.Text = text;
    s.IsSeparator = (text == 0);
    s.SubMenu = 0;
    s.CommandId = id;

    if (hasSubMenu)
    {
        s.SubMenu = new CGUIContextMenu(Environment, this, -1, core::CRect<int>(0, 0, 100, 100), false);
        s.SubMenu->setVisible(false);
    }

    Items.push_back(s);

    recalculateSize();
    return Items.size() - 1;
}

//! Adds a separator item to the menu
void CGUIContextMenu::addSeparator()
{
    addItem(0, true);
}

//! Returns text of the menu item.
const wchar_t* CGUIContextMenu::getItemText(int idx)
{
    if (idx < 0 || idx >= (int)Items.size())
        return 0;

    return Items[idx].Text.c_str();
}

//! Sets text of the menu item.
void CGUIContextMenu::setItemText(int idx, const wchar_t* text)
{
    if (idx < 0 || idx >= (int)Items.size())
        return;

    Items[idx].Text = text;
    recalculateSize();
}

//! Returns if a menu item is enabled
bool CGUIContextMenu::isItemEnabled(int idx)
{
    if (idx < 0 || idx >= (int)Items.size())
        return false;

    return Items[idx].Enabled;
}

//! Sets if the menu item should be enabled.
void CGUIContextMenu::setItemEnabled(int idx, bool enabled)
{
    if (idx < 0 || idx >= (int)Items.size())
        return;

    Items[idx].Enabled = enabled;
}

//! Removes a menu item
void CGUIContextMenu::removeItem(int idx)
{
    if (idx < 0 || idx >= (int)Items.size())
        return;

    if (Items[idx].SubMenu)
    {
        Items[idx].SubMenu->drop();
        Items[idx].SubMenu = 0;
    }

    Items.erase(Items.begin() + idx);
    recalculateSize();
}

//! Removes all menu items
void CGUIContextMenu::removeAllItems()
{
    for (int i = 0; i < (int)Items.size(); ++i)
        if (Items[i].SubMenu)
            Items[i].SubMenu->drop();

    Items.clear();
    recalculateSize();
}

//! called if an event happened.
bool CGUIContextMenu::OnEvent(const event::SEvent& event)
{
    if (!IsEnabled)
        return Parent ? Parent->OnEvent(event) : false;

    switch (event.EventType)
    {
    case event::EET_GUI_EVENT:
        switch (event.GUIEvent.EventType)
        {
        case EGET_ELEMENT_FOCUS_LOST:
            remove();
            return true;
        default:
            break;
        }
        break;
    case event::EET_MOUSE_INPUT_EVENT:
        switch (event.MouseInput.Event)
        {
        case event::EMIE_LMOUSE_LEFT_UP:
            {
                int t = sendClick(core::CPosition2d<int>(event.MouseInput.X, event.MouseInput.Y));
                if ((t == 0 || t == 1) && Environment->hasFocus(this))
                    Environment->removeFocus(this);
            }
            return true;
        case event::EMIE_LMOUSE_PRESSED_DOWN:
            return true;
        case event::EMIE_MOUSE_MOVED:
            if (Environment->hasFocus(this))
                highlight(core::CPosition2d<int>(event.MouseInput.X, event.MouseInput.Y));
            return true;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return Parent ? Parent->OnEvent(event) : false;
}

//! Sets the visible state of this element.
void CGUIContextMenu::setVisible(bool visible)
{
    HighLighted = -1;
    for (int j = 0; j < (int)Items.size(); ++j)
        if (Items[j].SubMenu)
            Items[j].SubMenu->setVisible(false);

    IGUIElement::setVisible(visible);
}

//! sends a click. Returns:
//! 0 if click went outside of the element,
//! 1 if a valid button was clicked,
//! 2 if a nonclickable element was clicked
int CGUIContextMenu::sendClick(core::CPosition2d<int> p)
{
    int t = 0;

    // get number of open submenu
    int openmenu = -1;
    int j;
    for (j = 0; j < (int)Items.size(); ++j)
        if (Items[j].SubMenu && Items[j].SubMenu->isVisible())
        {
            openmenu = j;
            break;
        }

    // delegate click operation to submenu
    if (openmenu != -1)
    {
        t = Items[j].SubMenu->sendClick(p);
        if (t != 0)
            return t; // clicked something
    }

    // check click on myself
    if (AbsoluteRect.isPointInside(p) && HighLighted >= 0 && HighLighted < (int)Items.size())
    {
        if (!Items[HighLighted].Enabled || Items[HighLighted].IsSeparator || Items[HighLighted].SubMenu)
            return 2;

        event::SEvent event;
        event.EventType = event::EET_GUI_EVENT;
        event.GUIEvent.Caller = this;
        event.GUIEvent.EventType = EGET_MENU_ITEM_SELECTED;
        Parent->OnEvent(event);

        return 1;
    }

    return 0;
}

//! returns true, if an element was highlighted
bool CGUIContextMenu::highlight(core::CPosition2d<int> p)
{
    // get number of open submenu
    int openmenu = -1;
    int j;
    for (j = 0; j < (int)Items.size(); ++j)
        if (Items[j].SubMenu && Items[j].SubMenu->isVisible())
        {
            openmenu = j;
            break;
        }

    // delegate highlight operation to submenu
    if (openmenu != -1)
    {
        if (Items[j].SubMenu->highlight(p))
        {
            HighLighted = openmenu;
            return true;
        }
    }

    // highlight myself
    for (int i = 0; i < (int)Items.size(); ++i)
        if (getHRect(Items[i], AbsoluteRect).isPointInside(p))
        {
            HighLighted = i;

            // make submenus visible/invisible
            for (int j = 0; j < (int)Items.size(); ++j)
                if (Items[j].SubMenu)
                    Items[j].SubMenu->setVisible(j == i);
            return true;
        }

    HighLighted = openmenu;
    return false;
}

//! returns the item highlight-area
core::CRect<int> CGUIContextMenu::getHRect(SItem& i, core::CRect<int>& absolute)
{
    core::CRect<int> r = absolute;
    r.UpperLeftCorner.Y += i.PosY;
    r.LowerRightCorner.Y = r.UpperLeftCorner.Y + i.Dim.Height;
    return r;
}

//! Gets drawing rect of Item
core::CRect<int> CGUIContextMenu::getRect(SItem& i, core::CRect<int>& absolute)
{
    core::CRect<int> r = absolute;
    r.UpperLeftCorner.Y += i.PosY;
    r.LowerRightCorner.Y = r.UpperLeftCorner.Y + i.Dim.Height;
    r.UpperLeftCorner.X += 20;
    return r;
}

//! draws the element and its children
void CGUIContextMenu::draw()
{
    if (!IsVisible)
        return;

    IGUISkin* skin = Environment->getSkin();
    IGUIFont* font = skin->getFont();
    IGUIFont* defaultFont = Environment->getBuiltInFont();
    video::IVideoDriver* driver = Environment->getVideoDriver();

    core::CRect<int> rect = AbsoluteRect;
    core::CRect<int>* clip = 0;

    // draw frame

    rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + 1;
    driver->draw2DRectangle(skin->getColor(EGDC_3D_HIGH_LIGHT), rect, clip);

    rect.LowerRightCorner.Y = AbsoluteRect.LowerRightCorner.Y;
    rect.LowerRightCorner.X = rect.UpperLeftCorner.X + 1;
    driver->draw2DRectangle(skin->getColor(EGDC_3D_HIGH_LIGHT), rect, clip);

    rect.UpperLeftCorner.X = AbsoluteRect.LowerRightCorner.X - 1;
    rect.LowerRightCorner.X = AbsoluteRect.LowerRightCorner.X;
    rect.UpperLeftCorner.Y = AbsoluteRect.UpperLeftCorner.Y;
    rect.LowerRightCorner.Y = AbsoluteRect.LowerRightCorner.Y;
    driver->draw2DRectangle(skin->getColor(EGDC_3D_DARK_SHADOW), rect, clip);

    rect.UpperLeftCorner.X -= 1;
    rect.LowerRightCorner.X -= 1;
    rect.UpperLeftCorner.Y += 1;
    rect.LowerRightCorner.Y -= 1;
    driver->draw2DRectangle(skin->getColor(EGDC_3D_SHADOW), rect, clip);

    rect.UpperLeftCorner.X = AbsoluteRect.UpperLeftCorner.X;
    rect.UpperLeftCorner.Y = AbsoluteRect.LowerRightCorner.Y - 1;
    rect.LowerRightCorner.Y = AbsoluteRect.LowerRightCorner.Y;
    rect.LowerRightCorner.X = AbsoluteRect.LowerRightCorner.X;
    driver->draw2DRectangle(skin->getColor(EGDC_3D_DARK_SHADOW), rect, clip);

    rect.UpperLeftCorner.X += 1;
    rect.LowerRightCorner.X -= 1;
    rect.UpperLeftCorner.Y -= 1;
    rect.LowerRightCorner.Y -= 1;
    driver->draw2DRectangle(skin->getColor(EGDC_3D_SHADOW), rect, clip);

    rect = AbsoluteRect;
    rect.UpperLeftCorner.X += 1;
    rect.UpperLeftCorner.Y += 1;
    rect.LowerRightCorner.X -= 2;
    rect.LowerRightCorner.Y -= 2;
    driver->draw2DRectangle(skin->getColor(EGDC_3D_FACE), rect, clip);

    // loop through all menu items

    rect = AbsoluteRect;
    int y = AbsoluteRect.UpperLeftCorner.Y;

    for (int i = 0; i < (int)Items.size(); ++i)
    {
        if (Items[i].IsSeparator)
        {
            // draw separator
            rect = AbsoluteRect;
            rect.UpperLeftCorner.Y += Items[i].PosY + 3;
            rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + 1;
            rect.UpperLeftCorner.X += 5;
            rect.LowerRightCorner.X -= 5;
            driver->draw2DRectangle(skin->getColor(EGDC_3D_SHADOW), rect, clip);

            rect.LowerRightCorner.Y += 1;
            rect.UpperLeftCorner.Y += 1;
            driver->draw2DRectangle(skin->getColor(EGDC_3D_HIGH_LIGHT), rect, clip);

            y += 10;
        }
        else
        {
            rect = getRect(Items[i], AbsoluteRect);

            // draw highlighted

            if (i == HighLighted && Items[i].Enabled)
            {
                core::CRect<int> r = AbsoluteRect;
                r.LowerRightCorner.Y = rect.LowerRightCorner.Y;
                r.UpperLeftCorner.Y = rect.UpperLeftCorner.Y;
                r.LowerRightCorner.X -= 5;
                r.UpperLeftCorner.X += 5;
                driver->draw2DRectangle(skin->getColor(EGDC_HIGH_LIGHT), r, clip);
            }

            // draw text

            EGUI_DEFAULT_COLOR c = EGDC_BUTTON_TEXT;

            if (i == HighLighted)
                c = EGDC_HIGH_LIGHT_TEXT;

            if (!Items[i].Enabled)
                c = EGDC_GRAY_TEXT;

            font->draw(Items[i].Text.c_str(), rect, skin->getColor(c), EFHA_LEFT, EFVA_CENTER, clip);

            // draw submenu symbol
            if (Items[i].SubMenu && defaultFont)
            {
                core::CRect<int> r = rect;
                r.UpperLeftCorner.X = r.LowerRightCorner.X - 15;

                defaultFont->draw(GUI_ICON_CURSOR_RIGHT, r, skin->getColor(c), EFHA_CENTER, EFVA_CENTER, clip);
            }
        }
    }

    IGUIElement::draw();
}

void CGUIContextMenu::recalculateSize()
{
    IGUISkin* skin = Environment->getSkin();
    IGUIFont* font = skin->getFont();

    if (!font)
        return;

    core::CRect<int> rect;
    rect.UpperLeftCorner = RelativeRect.UpperLeftCorner;
    int width = 100;
    int height = 3;
    int i;

    for (i = 0; i < (int)Items.size(); ++i)
    {
        if (Items[i].IsSeparator)
        {
            Items[i].Dim.Width = 100;
            Items[i].Dim.Height = 10;
        }
        else
        {
            Items[i].Dim = font->getDimension(Items[i].Text.c_str());
            Items[i].Dim.Width += 40;

            if (Items[i].Dim.Width > width)
                width = Items[i].Dim.Width;
        }

        Items[i].PosY = height;
        height += Items[i].Dim.Height;
    }

    height += 5;

    if (height < 10)
        height = 10;

    rect.LowerRightCorner.X = RelativeRect.UpperLeftCorner.X + width;
    rect.LowerRightCorner.Y = RelativeRect.UpperLeftCorner.Y + height;
    RelativeRect = rect;
    updateAbsolutePosition();

    // recalculate submenus
    for (i = 0; i < (int)Items.size(); ++i)
        if (Items[i].SubMenu)
        {
            // move submenu
            int w = Items[i].SubMenu->getAbsolutePosition().getWidth();
            int h = Items[i].SubMenu->getAbsolutePosition().getHeight();

            Items[i].SubMenu->setRelativePosition(
                core::CRect<int>(width - 5, Items[i].PosY, width + w - 5, Items[i].PosY + h));
        }
}

//! Returns the selected item in the menu
int CGUIContextMenu::getSelectedItem()
{
    return HighLighted;
}

//! \return Returns a pointer to the submenu of an item.
IGUIContextMenu* CGUIContextMenu::getSubMenu(int idx)
{
    if (idx < 0 || idx >= (int)Items.size())
        return 0;

    return Items[idx].SubMenu;
}

//! Returns command id of a menu item
int CGUIContextMenu::getItemCommandId(int idx)
{
    if (idx < 0 || idx >= (int)Items.size())
        return -1;

    return Items[idx].CommandId;
}

//! Sets the command id of a menu item
void CGUIContextMenu::setItemCommandId(int idx, int id)
{
    if (idx < 0 || idx >= (int)Items.size())
        return;

    Items[idx].CommandId = id;
}

} // end namespace gui
} // end namespace daisy
