// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIMenu.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#include "CGUIMenu.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/gui/IGUISkin.h"
#include "ox/video/IVideoDriver.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

//! constructor
CGUIMenu::CGUIMenu(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
    ox::core::CRect<int> rectangle)
    : CGUIContextMenu(environment, parent, id, rectangle, false)
{
    recalculateSize();
}

//! destructor
CGUIMenu::~CGUIMenu()
{
}

//! draws the element and its children
void CGUIMenu::draw()
{
    if (!IsVisible)
        return;

    ox::gui::IGUISkin* skin = Environment->getSkin();
    ox::gui::IGUIFont* font = skin->getFont();
    ox::gui::IGUIFont* defaultFont = Environment->getBuiltInFont();
    ox::video::IVideoDriver* driver = Environment->getVideoDriver();

    ox::core::CRect<int> rect = AbsoluteRect;
    ox::core::CRect<int>* clip = &AbsoluteClippingRect;

    // draw frame
    rect.UpperLeftCorner.X = AbsoluteRect.UpperLeftCorner.X;
    rect.UpperLeftCorner.Y = AbsoluteRect.LowerRightCorner.Y - 1;
    rect.LowerRightCorner.Y = AbsoluteRect.LowerRightCorner.Y;
    rect.LowerRightCorner.X = AbsoluteRect.LowerRightCorner.X;
    driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_SHADOW), rect, clip);

    rect = AbsoluteRect;
    rect.LowerRightCorner.Y -= 1;
    driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_FACE), rect, clip);

    // loop through all menu items
    rect = AbsoluteRect;

    for (int i = 0; i < (int)Items.size(); ++i)
    {
        if (!Items[i].IsSeparator)
        {
            rect = getRect(Items[i], AbsoluteRect);

            // draw highlighted
            if (i == HighLighted && Items[i].Enabled)
            {
                ox::core::CRect<int> tr;

                // draw upper highlight
                tr = rect;
                tr.LowerRightCorner.X -= 2;
                tr.LowerRightCorner.Y = tr.UpperLeftCorner.Y + 1;
                tr.UpperLeftCorner.X += 1;
                driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_SHADOW), tr, &AbsoluteClippingRect);

                // draw left highlight
                tr = rect;
                tr.LowerRightCorner.X = tr.UpperLeftCorner.X + 1;
                tr.UpperLeftCorner.Y += 1;
                driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_DARK_SHADOW), tr, &AbsoluteClippingRect);

                // draw grey background
                tr = rect;
                tr.UpperLeftCorner.X += 1;
                tr.UpperLeftCorner.Y += 1;
                tr.LowerRightCorner.X -= 2;
                driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_DARK_SHADOW), tr, &AbsoluteClippingRect);

                // draw right middle gray shadow
                tr.LowerRightCorner.X += 1;
                tr.UpperLeftCorner.X = tr.LowerRightCorner.X - 1;
                driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_HIGH_LIGHT), tr, &AbsoluteClippingRect);
            }

            // draw text
            ox::gui::EGUI_DEFAULT_COLOR c = ox::gui::EGDC_BUTTON_TEXT;

            if (i == HighLighted)
                c = ox::gui::EGDC_HIGH_LIGHT_TEXT;

            if (!Items[i].Enabled)
                c = ox::gui::EGDC_GRAY_TEXT;

            font->draw(Items[i].Text.c_str(), rect, skin->getColor(c), ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER,
                clip);
        }
    }

    IGUIElement::draw();
}

//! called if an event happened.
bool CGUIMenu::OnEvent(const ox::event::SEvent& event)
{
    if (!IsEnabled)
        return Parent ? Parent->OnEvent(event) : false;

    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
        switch (event.GUIEvent.EventType)
        {
        case ox::gui::EGET_ELEMENT_FOCUS_LOST:
            closeAllSubMenus();
            return true;

        default:
            break;
        }
        break;

    case ox::event::EET_MOUSE_INPUT_EVENT:
        switch (event.MouseInput.Event)
        {
        case ox::event::EMIE_LMOUSE_LEFT_UP:
            {
                ox::core::CPosition2d<int> p(event.MouseInput.X, event.MouseInput.Y);
                if (AbsoluteRect.isPointInside(p))
                {
                    if (HighLighted != -1)
                        Environment->removeFocus(this);
                    else
                        highlight(ox::core::CPosition2d<int>(event.MouseInput.X, event.MouseInput.Y));
                }
                else
                {
                    int t = sendClick(p);
                    if ((t == 0 || t == 1) && Environment->hasFocus(this))
                        Environment->removeFocus(this);
                }
            }
            return true;

        case ox::event::EMIE_LMOUSE_PRESSED_DOWN:
            if (!Environment->hasFocus(this))
            {
                Environment->setFocus(this);
                if (Parent)
                    Parent->bringToFront(this);
            }
            return true;

        case ox::event::EMIE_MOUSE_MOVED:
            if (Environment->hasFocus(this))
                highlight(ox::core::CPosition2d<int>(event.MouseInput.X, event.MouseInput.Y));
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

void CGUIMenu::recalculateSize()
{
    ox::gui::IGUISkin* skin = Environment->getSkin();
    ox::gui::IGUIFont* font = skin->getFont();

    if (!font)
        return;

    ox::core::CRect<int> rect;
    rect.UpperLeftCorner.X = 0;
    rect.UpperLeftCorner.Y = 0;
    int height = font->getDimension(L"A").Height + 5;
    int width = 0;
    int i;

    for (i = 0; i < (int)Items.size(); ++i)
    {
        if (Items[i].IsSeparator)
        {
            Items[i].Dim.Width = 0;
            Items[i].Dim.Height = height;
        }
        else
        {
            Items[i].Dim = font->getDimension(Items[i].Text.c_str());
            Items[i].Dim.Width += 20;
        }

        Items[i].PosY = width;
        width += Items[i].Dim.Width;
    }

    if (Parent)
        width = Parent->getAbsolutePosition().getWidth();

    rect.LowerRightCorner.X = width;
    rect.LowerRightCorner.Y = height;
    RelativeRect = rect;
    updateAbsolutePosition();

    // recalculate submenus
    for (i = 0; i < (int)Items.size(); ++i)
    {
        if (Items[i].SubMenu)
        {
            // move submenu
            int w = Items[i].SubMenu->getAbsolutePosition().getWidth();
            int h = Items[i].SubMenu->getAbsolutePosition().getHeight();

            Items[i].SubMenu->setRelativePosition(
                ox::core::CRect<int>(Items[i].PosY, height, Items[i].PosY + w - 5, height + h));
        }
    }
}

//! returns the item highlight-area
ox::core::CRect<int> CGUIMenu::getHRect(SItem& i, ox::core::CRect<int>& absolute)
{
    ox::core::CRect<int> r = absolute;
    r.UpperLeftCorner.X += i.PosY;
    r.LowerRightCorner.X = r.UpperLeftCorner.X + i.Dim.Width;
    return r;
}

//! Gets drawing rect of Item
ox::core::CRect<int> CGUIMenu::getRect(SItem& i, ox::core::CRect<int>& absolute)
{
    return getHRect(i, absolute);
}

void CGUIMenu::closeAllSubMenus()
{
    for (int i = 0; i < (int)Items.size(); ++i)
        if (Items[i].SubMenu)
            Items[i].SubMenu->setVisible(false);

    HighLighted = -1;
}

void CGUIMenu::updateAbsolutePosition()
{
    if (Parent)
        RelativeRect.LowerRightCorner.X = Parent->getAbsolutePosition().getWidth();

    IGUIElement::updateAbsolutePosition();
}

} // end namespace gui
} // end namespace daisy
