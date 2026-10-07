// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUITabControl.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye adds a sprite skin to the tab control and the tab button row (CGUITabButtonRow.h) in this file.

#include "CGUITabControl.h"
#include "CGUITabButtonRow.h"
#include "ox/gui/IGUILayoutInline.h"
#include "ox/gui/IGUISkin.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/ISpriteAnimationState.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

//! Clips rect against other, as Irrlicht's rect::clipAgainst.
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

// ------------------------------------------------------------------
// Tab
// ------------------------------------------------------------------

//! constructor
CGUITab::CGUITab(int number, ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent,
    const ox::core::CRect<int>& rectangle, int id)
    : IGUITab(environment, parent, id, rectangle), Number(number), DrawBackground(false), BackColor(0)
{
}

//! destructor
CGUITab::~CGUITab()
{
}

//! Returns number of tab in tabcontrol. Can be accessed
//! later IGUITabControl::getTab() by this number.
int CGUITab::getNumber()
{
    return Number;
}

//! Sets the number
void CGUITab::setNumber(int n)
{
    Number = n;
}

//! draws the element and its children
void CGUITab::draw()
{
    if (!IsVisible)
        return;

    ox::video::IVideoDriver* driver = Environment->getVideoDriver();

    if (DrawBackground)
        driver->draw2DRectangle(BackColor, AbsoluteRect, &AbsoluteClippingRect);

    IGUIElement::draw();
}

//! sets if the tab should draw its background
void CGUITab::setDrawBackground(bool draw)
{
    DrawBackground = draw;
}

//! sets the color of the background, if it should be drawn.
void CGUITab::setBackgroundColor(ox::video::SColor c)
{
    BackColor = c;
}

// ------------------------------------------------------------------
// Tabcontrol
// ------------------------------------------------------------------

//! constructor
CGUITabControl::CGUITabControl(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent,
    const ox::core::CRect<int>& rectangle, bool fillbackground, bool border, int id)
    : IGUITabControl(environment, parent, id, rectangle), ActiveTab(-1), Border(border),
      FillBackground(fillbackground)
{
    for (int i = 0; i < ETCA_COUNT; ++i)
        Animations[i] = 0;

    TextColor = Environment->getSkin()->getColor(ox::gui::EGDC_BUTTON_TEXT);
}

//! Adds a tab
ox::gui::IGUITab* CGUITabControl::addTab(wchar_t* caption, int id)
{
    ox::gui::IGUISkin* skin = Environment->getSkin();
    if (!skin)
        return 0;

    int tabheight = skin->getSize(ox::gui::EGDS_BUTTON_HEIGHT) + 2;
    ox::core::CRect<int> r(1, tabheight, AbsoluteRect.getWidth() - 1, AbsoluteRect.getHeight() - 1);

    CGUITab* tab = new CGUITab(Tabs.size(), Environment, this, r, id);

    tab->setText(caption);
    tab->setVisible(false);
    Tabs.push_back(tab);

    if (ActiveTab == -1)
    {
        ActiveTab = 0;
        tab->setVisible(true);
    }

    return tab;
}

//! Returns amount of tabs in the tabcontrol
int CGUITabControl::getTabcount()
{
    return Tabs.size();
}

//! Returns a tab based on zero based index
ox::gui::IGUITab* CGUITabControl::getTab(int idx)
{
    if (idx < 0 || idx >= (int)Tabs.size())
        return 0;

    return Tabs[idx];
}

//! called if an event happened.
bool CGUITabControl::OnEvent(const ox::event::SEvent& event)
{
    if (!IsEnabled)
        return Parent ? Parent->OnEvent(event) : false;

    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
        switch (event.GUIEvent.EventType)
        {
        case ox::gui::EGET_ELEMENT_FOCUS_LOST:
            return true;
        default:
            break;
        }
        break;
    case ox::event::EET_MOUSE_INPUT_EVENT:
        switch (event.MouseInput.Event)
        {
        case ox::event::EMIE_LMOUSE_PRESSED_DOWN:
            Environment->setFocus(this);
            return true;
        case ox::event::EMIE_LMOUSE_LEFT_UP:
            Environment->removeFocus(this);
            selectTab(ox::core::CPosition2d<int>(event.MouseInput.X, event.MouseInput.Y));
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

void CGUITabControl::selectTab(ox::core::CPosition2d<int> p)
{
    if (!Animations[ETCA_TAB])
    {
        ox::gui::IGUISkin* skin = Environment->getSkin();
        ox::gui::IGUIFont* font = skin->getFont();

        ox::core::CRect<int> frameRect(AbsoluteRect);

        int tabheight = skin->getSize(ox::gui::EGDS_BUTTON_HEIGHT);
        frameRect.UpperLeftCorner.Y += 2;
        frameRect.LowerRightCorner.Y = frameRect.UpperLeftCorner.Y + tabheight;
        int pos = frameRect.UpperLeftCorner.X + 2;

        for (int i = 0; i < (int)Tabs.size(); ++i)
        {
            // get Text
            const wchar_t* text = 0;
            if (Tabs[i])
                text = Tabs[i]->getText();

            // get text length
            int len = font->getDimension(text).Width + 20;
            frameRect.UpperLeftCorner.X = pos;
            frameRect.LowerRightCorner.X = frameRect.UpperLeftCorner.X + len;
            pos += len;

            if (frameRect.isPointInside(p))
            {
                setActiveTab(i);
                return;
            }
        }
    }
    else
    {
        // the skinned tabs all have the size of the tab animation
        p.X -= AbsoluteRect.UpperLeftCorner.X;
        p.Y -= AbsoluteRect.UpperLeftCorner.Y;
        int index = p.X / Rects[ETCA_TAB].getWidth();
        if (index >= 0 && p.Y >= 0 && index < (int)Tabs.size() && p.Y < Rects[ETCA_TAB].getHeight())
            setActiveTab(index);
    }
}

//! draws the element and its children
void CGUITabControl::draw()
{
    if (!IsVisible)
        return;

    ox::gui::IGUISkin* skin = Environment->getSkin();
    ox::gui::IGUIFont* font = skin->getFont();
    ox::video::IVideoDriver* driver = Environment->getVideoDriver();

    ox::core::CRect<int> frameRect(AbsoluteRect);

    if (Tabs.empty())
        driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_HIGH_LIGHT), frameRect, &AbsoluteClippingRect);

    if (!font)
        return;

    if (Animations[ETCA_BACKGROUND])
    {
        // the skin: corners, tiled edges, tiled background, then the tabs over the tiled top edge
        int x = frameRect.UpperLeftCorner.X;
        int y = frameRect.UpperLeftCorner.Y;
        const ox::video::SColor white(0xffffffff);

        Animations[ETCA_TOP_LEFT]->draw(ox::core::CPosition2d<int>(Rects[ETCA_TOP_LEFT].UpperLeftCorner.X + x,
            Rects[ETCA_TOP_LEFT].UpperLeftCorner.Y + y), &AbsoluteClippingRect, white);
        Animations[ETCA_TOP_RIGHT]->draw(ox::core::CPosition2d<int>(Rects[ETCA_TOP_RIGHT].UpperLeftCorner.X + x,
            Rects[ETCA_TOP_RIGHT].UpperLeftCorner.Y + y), &AbsoluteClippingRect, white);
        Animations[ETCA_BOTTOM_LEFT]->draw(ox::core::CPosition2d<int>(Rects[ETCA_BOTTOM_LEFT].UpperLeftCorner.X + x,
            Rects[ETCA_BOTTOM_LEFT].UpperLeftCorner.Y + y), &AbsoluteClippingRect, white);
        Animations[ETCA_BOTTOM_RIGHT]->draw(ox::core::CPosition2d<int>(Rects[ETCA_BOTTOM_RIGHT].UpperLeftCorner.X + x,
            Rects[ETCA_BOTTOM_RIGHT].UpperLeftCorner.Y + y), &AbsoluteClippingRect, white);

        ox::core::CRect<int> r(Rects[ETCA_BOTTOM].UpperLeftCorner.X + x, Rects[ETCA_BOTTOM].UpperLeftCorner.Y + y,
            Rects[ETCA_BOTTOM].LowerRightCorner.X + x, Rects[ETCA_BOTTOM].LowerRightCorner.Y + y);
        clipAgainst(r, AbsoluteClippingRect);
        int step = Animations[ETCA_BOTTOM]->getFrameSize(0).Width;
        for (int i = r.UpperLeftCorner.X; i < r.LowerRightCorner.X; i += step)
            Animations[ETCA_BOTTOM]->draw(ox::core::CPosition2d<int>(i, r.UpperLeftCorner.Y), &r, white);

        r = ox::core::CRect<int>(Rects[ETCA_LEFT].UpperLeftCorner.X + x, Rects[ETCA_LEFT].UpperLeftCorner.Y + y,
            Rects[ETCA_LEFT].LowerRightCorner.X + x, Rects[ETCA_LEFT].LowerRightCorner.Y + y);
        clipAgainst(r, AbsoluteClippingRect);
        step = Animations[ETCA_LEFT]->getFrameSize(0).Height;
        for (int i = r.UpperLeftCorner.Y; i < r.LowerRightCorner.Y; i += step)
            Animations[ETCA_LEFT]->draw(ox::core::CPosition2d<int>(r.UpperLeftCorner.X, i), &r, white);

        r = ox::core::CRect<int>(Rects[ETCA_RIGHT].UpperLeftCorner.X + x, Rects[ETCA_RIGHT].UpperLeftCorner.Y + y,
            Rects[ETCA_RIGHT].LowerRightCorner.X + x, Rects[ETCA_RIGHT].LowerRightCorner.Y + y);
        clipAgainst(r, AbsoluteClippingRect);
        step = Animations[ETCA_RIGHT]->getFrameSize(0).Height;
        for (int i = r.UpperLeftCorner.Y; i < r.LowerRightCorner.Y; i += step)
            Animations[ETCA_RIGHT]->draw(ox::core::CPosition2d<int>(r.UpperLeftCorner.X, i), &r, white);

        r = ox::core::CRect<int>(Rects[ETCA_BACKGROUND].UpperLeftCorner.X + x,
            Rects[ETCA_BACKGROUND].UpperLeftCorner.Y + y, Rects[ETCA_BACKGROUND].LowerRightCorner.X + x,
            Rects[ETCA_BACKGROUND].LowerRightCorner.Y + y);
        clipAgainst(r, AbsoluteClippingRect);
        int width = Animations[ETCA_BACKGROUND]->getFrameSize(0).Width;
        int height = Animations[ETCA_BACKGROUND]->getFrameSize(0).Height;
        for (int j = r.UpperLeftCorner.Y; j < r.LowerRightCorner.Y; j += height)
            for (int i = r.UpperLeftCorner.X; i < r.LowerRightCorner.X; i += width)
                Animations[ETCA_BACKGROUND]->draw(ox::core::CPosition2d<int>(i, j), &r, white);

        // the tabs
        r = ox::core::CRect<int>(Rects[ETCA_TOP].UpperLeftCorner.X + x, Rects[ETCA_TOP].UpperLeftCorner.Y + y,
            Rects[ETCA_TOP].LowerRightCorner.X + x, Rects[ETCA_TOP].LowerRightCorner.Y + y);
        int tabWidth = Animations[ETCA_TAB]->getFrameSize(0).Width;
        int tabHeight = Animations[ETCA_TAB]->getFrameSize(0).Height;
        int left = 0;
        int right = 0;
        int offset = 0;
        for (int i = 0; i < (int)Tabs.size(); ++i)
        {
            int tabX = r.UpperLeftCorner.X + offset;
            Animations[ETCA_TAB]->draw(ox::core::CPosition2d<int>(tabX, y), &AbsoluteClippingRect, white);
            font->draw(Tabs[i]->getText(), ox::core::CRect<int>(tabX, y, tabX + tabWidth, tabHeight + y),
                TextColor, ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER, &AbsoluteClippingRect);

            if (i == ActiveTab)
                left = tabX;
            if (i == ActiveTab)
                right = tabX + tabWidth;
            offset += tabWidth;
        }

        // the top edge left and right of the active tab
        width = Animations[ETCA_TOP]->getFrameSize(0).Width;
        if (left > r.UpperLeftCorner.X)
        {
            r.LowerRightCorner.X = left;
            clipAgainst(r, AbsoluteClippingRect);
            for (int i = r.UpperLeftCorner.X; i < r.LowerRightCorner.X; i += width)
                Animations[ETCA_TOP]->draw(ox::core::CPosition2d<int>(i, r.UpperLeftCorner.Y), &r, white);
        }

        r = ox::core::CRect<int>(right, y + Rects[ETCA_TOP].UpperLeftCorner.Y, x + Rects[ETCA_TOP].LowerRightCorner.X,
            y + Rects[ETCA_TOP].LowerRightCorner.Y);
        clipAgainst(r, AbsoluteClippingRect);
        for (int i = r.UpperLeftCorner.X; i < r.LowerRightCorner.X; i += width)
            Animations[ETCA_TOP]->draw(ox::core::CPosition2d<int>(i, r.UpperLeftCorner.Y), &r, white);
    }
    else
    {
        int tabheight = skin->getSize(ox::gui::EGDS_BUTTON_HEIGHT);
        frameRect.UpperLeftCorner.Y += 2;
        frameRect.LowerRightCorner.Y = frameRect.UpperLeftCorner.Y + tabheight;
        ox::core::CRect<int> tr;
        int pos = frameRect.UpperLeftCorner.X + 2;

        // left and right pos of the active tab
        int left = 0;
        int right = 0;
        const wchar_t* activetext = 0;

        for (int i = 0; i < (int)Tabs.size(); ++i)
        {
            // get Text
            const wchar_t* text = 0;
            if (Tabs[i])
                text = Tabs[i]->getText();

            // get text length
            int len = font->getDimension(text).Width + 20;
            frameRect.UpperLeftCorner.X = pos;
            frameRect.LowerRightCorner.X = frameRect.UpperLeftCorner.X + len;
            pos += len;

            if (i == ActiveTab)
            {
                left = frameRect.UpperLeftCorner.X;
                right = frameRect.LowerRightCorner.X;
                activetext = text;
            }
            else
            {
                // draw upper highlight
                tr = frameRect;
                tr.LowerRightCorner.X -= 2;
                tr.LowerRightCorner.Y = tr.UpperLeftCorner.Y + 1;
                tr.UpperLeftCorner.X += 1;
                driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_HIGH_LIGHT), tr, &AbsoluteClippingRect);

                // draw left highlight
                tr = frameRect;
                tr.LowerRightCorner.X = tr.UpperLeftCorner.X + 1;
                tr.UpperLeftCorner.Y += 1;
                driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_HIGH_LIGHT), tr, &AbsoluteClippingRect);

                // draw grey background
                tr = frameRect;
                tr.UpperLeftCorner.X += 1;
                tr.UpperLeftCorner.Y += 1;
                tr.LowerRightCorner.X -= 2;
                driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_FACE), tr, &AbsoluteClippingRect);

                // draw right middle gray shadow
                tr.LowerRightCorner.X += 1;
                tr.UpperLeftCorner.X = tr.LowerRightCorner.X - 1;
                driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_SHADOW), tr, &AbsoluteClippingRect);

                tr.LowerRightCorner.X += 1;
                tr.UpperLeftCorner.X += 1;
                tr.UpperLeftCorner.Y += 1;
                driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_DARK_SHADOW), tr, &AbsoluteClippingRect);

                // draw text
                font->draw(text, frameRect, TextColor, ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER,
                    &AbsoluteClippingRect);
            }
        }

        // draw active tab
        if (left != 0 && right != 0)
        {
            frameRect.UpperLeftCorner.X = left - 2;
            frameRect.LowerRightCorner.X = right + 2;
            frameRect.UpperLeftCorner.Y -= 2;

            // draw upper highlight
            tr = frameRect;
            tr.LowerRightCorner.X -= 2;
            tr.LowerRightCorner.Y = tr.UpperLeftCorner.Y + 1;
            tr.UpperLeftCorner.X += 1;
            driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_HIGH_LIGHT), tr, &AbsoluteClippingRect);

            // draw left highlight
            tr = frameRect;
            tr.LowerRightCorner.X = tr.UpperLeftCorner.X + 1;
            tr.UpperLeftCorner.Y += 1;
            driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_HIGH_LIGHT), tr, &AbsoluteClippingRect);

            // draw grey background
            tr = frameRect;
            tr.UpperLeftCorner.X += 1;
            tr.UpperLeftCorner.Y += 1;
            tr.LowerRightCorner.X -= 2;
            driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_FACE), tr, &AbsoluteClippingRect);

            // draw right middle gray shadow
            tr.LowerRightCorner.X += 1;
            tr.UpperLeftCorner.X = tr.LowerRightCorner.X - 1;
            driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_SHADOW), tr, &AbsoluteClippingRect);

            tr.LowerRightCorner.X += 1;
            tr.UpperLeftCorner.X += 1;
            tr.UpperLeftCorner.Y += 1;
            driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_DARK_SHADOW), tr, &AbsoluteClippingRect);

            // draw text
            font->draw(activetext, frameRect, TextColor, ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER,
                &AbsoluteClippingRect);

            // draw upper highlight frame
            tr.UpperLeftCorner.X = AbsoluteRect.UpperLeftCorner.X;
            tr.LowerRightCorner.X = left - 1;
            tr.UpperLeftCorner.Y = frameRect.LowerRightCorner.Y - 1;
            tr.LowerRightCorner.Y = frameRect.LowerRightCorner.Y;
            driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_HIGH_LIGHT), tr, &AbsoluteClippingRect);

            tr.UpperLeftCorner.X = right;
            tr.LowerRightCorner.X = AbsoluteRect.LowerRightCorner.X;
            driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_HIGH_LIGHT), tr, &AbsoluteClippingRect);
        }

        // draw border.
        if (Border)
        {
            // draw left hightlight
            tr = AbsoluteRect;
            tr.UpperLeftCorner.Y += skin->getSize(ox::gui::EGDS_BUTTON_HEIGHT) + 2;
            tr.LowerRightCorner.X = tr.UpperLeftCorner.X + 1;
            driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_HIGH_LIGHT), tr, &AbsoluteClippingRect);

            // draw right shadow
            tr.UpperLeftCorner.X = AbsoluteRect.LowerRightCorner.X - 1;
            tr.LowerRightCorner.X = tr.UpperLeftCorner.X + 1;
            driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_SHADOW), tr, &AbsoluteClippingRect);

            // draw lower shadow
            tr = AbsoluteRect;
            tr.UpperLeftCorner.Y = tr.LowerRightCorner.Y - 1;
            driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_SHADOW), tr, &AbsoluteClippingRect);
        }

        if (FillBackground)
        {
            tr = AbsoluteRect;
            tr.UpperLeftCorner.Y += skin->getSize(ox::gui::EGDS_BUTTON_HEIGHT) + 2;
            tr.LowerRightCorner.X -= 1;
            tr.UpperLeftCorner.X += 1;
            tr.LowerRightCorner.Y -= 1;
            driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_FACE), tr, &AbsoluteClippingRect);
        }
    }

    IGUIElement::draw();
}

//! Returns which tab is currently active
int CGUITabControl::getActiveTab()
{
    return ActiveTab;
}

//! Brings a tab to front.
bool CGUITabControl::setActiveTab(int idx)
{
    if (idx < 0 || idx >= (int)Tabs.size())
        return false;

    bool changed = (ActiveTab != idx);

    ActiveTab = idx;

    for (int i = 0; i < (int)Tabs.size(); ++i)
        if (Tabs[i])
            Tabs[i]->setVisible(i == ActiveTab);

    if (changed)
    {
        ox::event::SEvent event;
        event.EventType = ox::event::EET_GUI_EVENT;
        event.GUIEvent.Caller = this;
        event.GUIEvent.EventType = ox::gui::EGET_TAB_CHANGED;
        Parent->OnEvent(event);
    }

    return true;
}

//! Removes a child.
void CGUITabControl::removeChild(ox::gui::IGUIElement* child)
{
    bool isTab = false;
    int i = 0;

    // check if it is a tab
    for (i = 0; i < (int)Tabs.size();)
        if (Tabs[i] == child)
        {
            Tabs[i]->drop();
            Tabs.erase(Tabs.begin() + i);
            isTab = true;
        }
        else
            ++i;

    // reassign numbers
    if (isTab)
        for (i = 0; i < (int)Tabs.size(); ++i)
            if (Tabs[i])
                Tabs[i]->setNumber(i);

    // remove real element
    IGUIElement::removeChild(child);
}

void CGUITabControl::setAnimations(ox::video::ISpritePackage* package, const char* name)
{
    const char* TABCONTROL_ANIMATION_NAMES[ETCA_COUNT] = { "Background", "TopLeft", "TopRight", "BottomLeft",
        "BottomRight", "Top", "Left", "Right", "Bottom", "Tab" };

    for (int i = 0; i < ETCA_COUNT; ++i)
    {
        if (Animations[i])
        {
            Animations[i]->remove();
            Animations[i] = 0;
        }

        ox::core::CString<char> animation = name;
        animation += ox::core::CString<char>(TABCONTROL_ANIMATION_NAMES[i]);
        Animations[i] = package->addNewAnimationState(animation.c_str());
        if (Animations[i])
        {
            ox::core::CDimension2d<int> size = Animations[i]->getFrameSize(0);
            Rects[i] = ox::core::CRect<int>(0, 0, size.Width, size.Height);
        }
    }

    // place the pieces around the control, the top edge level with the bottom of the tabs
    int offset = Rects[ETCA_TAB].getHeight() - Rects[ETCA_TOP].getHeight();
    int width = RelativeRect.getWidth();
    int height = RelativeRect.getHeight();

    Rects[ETCA_TOP_LEFT].UpperLeftCorner.Y += offset;
    Rects[ETCA_TOP_LEFT].LowerRightCorner.Y += offset;
    Rects[ETCA_TOP_RIGHT].UpperLeftCorner.Y += offset;
    Rects[ETCA_TOP_RIGHT].LowerRightCorner.Y += offset;
    Rects[ETCA_TOP].UpperLeftCorner.Y += offset;
    Rects[ETCA_TOP].LowerRightCorner.Y += offset;

    Rects[ETCA_TOP_RIGHT].UpperLeftCorner.X = width - Rects[ETCA_TOP_RIGHT].getWidth();
    Rects[ETCA_TOP_RIGHT].LowerRightCorner.X = width;
    Rects[ETCA_TOP].UpperLeftCorner.X = Rects[ETCA_TOP_LEFT].getWidth();
    Rects[ETCA_TOP].LowerRightCorner.X = Rects[ETCA_TOP_RIGHT].UpperLeftCorner.X;

    Rects[ETCA_BOTTOM_LEFT].UpperLeftCorner.Y = height - Rects[ETCA_BOTTOM_LEFT].getHeight();
    Rects[ETCA_BOTTOM_LEFT].LowerRightCorner.Y = height;

    int cornerWidth = Rects[ETCA_BOTTOM_RIGHT].getWidth();
    Rects[ETCA_BOTTOM_RIGHT] = ox::core::CRect<int>(width - cornerWidth,
        Rects[ETCA_BOTTOM_LEFT].UpperLeftCorner.Y, width, height);
    Rects[ETCA_BOTTOM] = ox::core::CRect<int>(cornerWidth, Rects[ETCA_BOTTOM_LEFT].UpperLeftCorner.Y,
        Rects[ETCA_BOTTOM_RIGHT].UpperLeftCorner.X, height);

    int edgeWidth = Rects[ETCA_LEFT].getWidth();
    int top = Rects[ETCA_TOP_RIGHT].getHeight() + offset;
    int bottom = Rects[ETCA_BOTTOM_LEFT].UpperLeftCorner.Y;
    Rects[ETCA_LEFT] = ox::core::CRect<int>(0, top, edgeWidth, bottom);
    Rects[ETCA_RIGHT] = ox::core::CRect<int>(width - edgeWidth, top, width, bottom);
    Rects[ETCA_BACKGROUND] = ox::core::CRect<int>(edgeWidth, top, width - edgeWidth, bottom);
}

void CGUITabControl::setTextColor(ox::video::SColor color)
{
    TextColor = color;
}

CGUITabButtonRow::CGUITabButtonRow(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent,
    const ox::core::CRect<int>& rectangle, int id)
    : IGUITabButtonRow(environment, parent, id, rectangle), ActiveTab(-1), HoverTab(-1)
{
    Type = ox::gui::EGUIET_TAB_BUTTON_ROW;

    // the original clears ETCA_COUNT pointers, running over into Rects
    for (int i = 0; i < ETCA_COUNT; ++i)
        Animations[i] = 0;

    TextColor = Environment->getSkin()->getColor(ox::gui::EGDC_BUTTON_TEXT);
    HighlightColor = ox::video::SColor(0xff64dc64);
    TextFont = Environment->getSkin()->getFont();

    if (Environment->getSkin()->getSpritePackage())
        setAnimations(Environment->getSkin()->getSpritePackage(), "TabControl");
}

// ------------------------------------------------------------------
// Tab button row
// ------------------------------------------------------------------

CGUITabButtonRow::~CGUITabButtonRow()
{
    for (unsigned int i = 0; i < Tabs.size(); ++i)
        delete Tabs[i];
}

int CGUITabButtonRow::addTab(const wchar_t* caption, bool closable)
{
    int index = Tabs.size();

    STabRowTabInfo* tab = new STabRowTabInfo;
    tab->Caption = caption;
    tab->Closable = closable;
    tab->Highlighted = false;

    if (Tabs.empty())
        ActiveTab = 0;

    Tabs.push_back(tab);
    repositionTabs();

    return index;
}

//! Spreads the tabs over the row, overlapping them when they do not fit.
void CGUITabButtonRow::repositionTabs()
{
    if (Tabs.empty())
        return;

    int step = Rects[ETBRA_TAB_NORMAL].getWidth();
    int count = Tabs.size();
    int width = RelativeRect.getWidth();
    if (count * step > width)
    {
        // overlap the tabs to fit the row
        if (Tabs.size() > 1)
        {
            int space = width - step;
            if (space < 0)
                space = 0;
            step = space / (count - 1);
        }

        int x = 0;
        for (unsigned int i = 0; i < Tabs.size(); ++i)
        {
            Tabs[i]->X = x;
            x += step;
        }
    }
    else
    {
        int x = 0;
        for (unsigned int i = 0; i < Tabs.size(); ++i)
        {
            Tabs[i]->X = x;
            x += step;
        }
    }
}

void CGUITabButtonRow::setTabButtonHighlight(int index, bool highlight)
{
    if (index >= 0 && index < (int)Tabs.size())
        Tabs[index]->Highlighted = highlight;
}

void CGUITabButtonRow::setHighlightColor(ox::video::SColor color)
{
    HighlightColor = color;
}

int CGUITabButtonRow::getTabCount()
{
    return Tabs.size();
}

const wchar_t* CGUITabButtonRow::getTabCaption(int index)
{
    if (index >= 0 && index < (int)Tabs.size())
        return Tabs[index]->Caption.c_str();

    return L"";
}

void CGUITabButtonRow::setActiveTabButton(int index)
{
    ActiveTab = index;
    repositionTabs();
}

int CGUITabButtonRow::getActiveTabButton()
{
    return ActiveTab;
}

bool CGUITabButtonRow::OnEvent(const ox::event::SEvent& event)
{
    if (EventReceiver)
    {
        if (EventReceiver->OnEvent(event))
            return true;
    }

    if (event.EventType == ox::event::EET_GUI_EVENT)
    {
        if (event.GUIEvent.EventType == ox::gui::EGET_ELEMENT_FOCUS_LOST)
        {
            HoverTab = -1;
            return true;
        }
    }
    else if (event.EventType == ox::event::EET_MOUSE_INPUT_EVENT)
    {
        if (!AbsoluteRect.isPointInside(ox::core::CPosition2d<int>(event.MouseInput.X, event.MouseInput.Y)))
            HoverTab = -1;
        else if (event.MouseInput.Event == ox::event::EMIE_MOUSE_MOVED)
        {
            int x = event.MouseInput.X - AbsoluteRect.UpperLeftCorner.X;
            int tabWidth = Rects[ETBRA_TAB_NORMAL].getWidth();
            HoverClose = false;
            for (unsigned int i = 0; i < Tabs.size(); ++i)
            {
                if (x >= Tabs[i]->X && x < Tabs[i]->X + tabWidth)
                {
                    // the tabs left of the active one are covered by their right neighbour
                    if ((int)i < ActiveTab && x >= Tabs[i + 1]->X)
                        continue;

                    HoverTab = i;
                    HoverClose = ClosePosition.X < x - Tabs[i]->X;
                    return true;
                }
            }
            return true;
        }
        else if (event.MouseInput.Event == ox::event::EMIE_LMOUSE_LEFT_UP)
        {
            int x = event.MouseInput.X - AbsoluteRect.UpperLeftCorner.X;
            int tabWidth = Rects[ETBRA_TAB_NORMAL].getWidth();
            int count = Tabs.size();
            for (unsigned int i = 0; i < Tabs.size(); ++i)
            {
                if (x >= Tabs[i]->X && x < Tabs[i]->X + tabWidth)
                {
                    if ((int)i < ActiveTab && x >= Tabs[i + 1]->X)
                        continue;

                    int index = i;
                    if (index >= 0 && index < count)
                    {
                        if (index == ActiveTab && HoverClose && Tabs[index]->Closable)
                        {
                            if (Parent)
                            {
                                ox::event::SEvent closed;
                                closed.EventType = ox::event::EET_GUI_EVENT;
                                closed.GUIEvent.Caller = this;
                                closed.GUIEvent.EventType = ox::gui::EGET_TAB_CLOSED;
                                closed.GUIEvent.Index = index;
                                Parent->OnEvent(closed);
                            }

                            delete Tabs[index];
                            Tabs.erase(Tabs.begin() + index);
                            HoverTab = -1;
                            if (index >= (int)Tabs.size())
                                --index;
                        }

                        ActiveTab = index;
                        repositionTabs();

                        if (Parent)
                        {
                            ox::event::SEvent changed;
                            changed.EventType = ox::event::EET_GUI_EVENT;
                            changed.GUIEvent.Caller = this;
                            changed.GUIEvent.EventType = ox::gui::EGET_TAB_CHANGED;
                            Parent->OnEvent(changed);
                        }
                    }
                    return true;
                }
            }
            return true;
        }
        else if (event.MouseInput.Event == ox::event::EMIE_LMOUSE_PRESSED_DOWN)
            return true;
    }

    return Parent ? Parent->OnEvent(event) : false;
}

void CGUITabButtonRow::draw()
{
    if (!IsVisible)
        return;

    Environment->getSkin();
    ox::video::IVideoDriver* driver = Environment->getVideoDriver();
    ox::core::CRect<int> frameRect(AbsoluteRect);

    // forget a hover that the mouse left without an event
    if (HoverTab >= 0 && !frameRect.isPointInside(Environment->getMousePosition()))
        HoverTab = -1;

    if (Animations[ETBRA_TOP])
    {
        const ox::video::SColor white(0xffffffff);
        ox::core::CRect<int> hoverRect;
        ox::core::CRect<int> rect = frameRect;
        int tabWidth = Animations[ETBRA_TAB_NORMAL]->getFrameSize(0).Width;
        int tabBottom = Animations[ETBRA_TAB_NORMAL]->getFrameSize(0).Height + frameRect.UpperLeftCorner.Y;

        // the tabs left of the active one, then from the right down to the active one, which overlaps both
        bool ascending = true;
        bool done = false;
        int left = 0;
        int right = 0;
        for (int i = 0; !done && i < (int)Tabs.size();)
        {
            int index = i;
            bool up = true;
            if (ascending && i >= ActiveTab && ActiveTab >= 0)
            {
                index = Tabs.size() - 1;
                ascending = false;
            }
            if (!ascending)
            {
                up = false;
                done = index == ActiveTab;
            }

            int tabX = Tabs[index]->X + rect.UpperLeftCorner.X;
            if (index != HoverTab)
                Animations[ETBRA_TAB_NORMAL]->draw(ox::core::CPosition2d<int>(tabX, frameRect.UpperLeftCorner.Y),
                    &AbsoluteClippingRect, white);
            else
            {
                Animations[ETBRA_TAB_HIGHLIGHTED]->draw(ox::core::CPosition2d<int>(tabX,
                    frameRect.UpperLeftCorner.Y), &AbsoluteClippingRect, white);
                if (index == ActiveTab && Tabs[index]->Closable && Animations[ETBRA_CLOSE_NORMAL])
                    Animations[ETBRA_CLOSE_NORMAL + HoverClose]->draw(ox::core::CPosition2d<int>(
                        ClosePosition.X + tabX, ClosePosition.Y + frameRect.UpperLeftCorner.Y),
                        &AbsoluteClippingRect, white);
            }

            ox::core::CRect<int> textRect(tabX + 6, frameRect.UpperLeftCorner.Y, tabX + tabWidth - 6, tabBottom);
            if (index != HoverTab)
            {
                ox::core::CRect<int> clip(tabX + 6, frameRect.UpperLeftCorner.Y, tabX + tabWidth - 6, tabBottom);
                clipAgainst(clip, AbsoluteClippingRect);
                STabRowTabInfo* tab = Tabs[index];
                TextFont->draw(tab->Caption.c_str(), textRect, tab->Highlighted ? HighlightColor : TextColor,
                    ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER, &clip);
            }
            else
                hoverRect = textRect;

            if (index == ActiveTab)
                left = tabX;
            if (index == ActiveTab)
                right = tabX + tabWidth;

            i = up ? index + 1 : index - 1;
        }

        // the hovered caption over everything, shaded unless it is the active tab
        if (HoverTab >= 0)
        {
            ox::core::CRect<int> clip = hoverRect;
            clipAgainst(clip, AbsoluteClippingRect);
            if (HoverTab != ActiveTab)
            {
                ox::core::CDimension2d<int> size = TextFont->getDimension(Tabs[HoverTab]->Caption.c_str());
                ox::core::CRect<int> shade = hoverRect;
                int space = shade.getWidth() - size.Width;
                if (space > 0)
                {
                    space >>= 1;
                    shade.UpperLeftCorner.X += space - 1;
                    shade.LowerRightCorner.X -= space - 1;
                    shade.UpperLeftCorner.Y += 3;
                    shade.LowerRightCorner.Y -= 3;
                }
                clipAgainst(shade, clip);
                driver->draw2DRectangle(ox::video::SColor(0x80000000), shade, 0);
            }

            STabRowTabInfo* tab = Tabs[HoverTab];
            TextFont->draw(tab->Caption.c_str(), hoverRect, tab->Highlighted ? HighlightColor : TextColor,
                ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER, &clip);
        }

        // the top edge left and right of the active tab
        int width = Animations[ETBRA_TOP]->getFrameSize(0).Width;
        int top = Rects[ETBRA_TOP].UpperLeftCorner.Y - Rects[ETBRA_TOP].LowerRightCorner.Y +
            frameRect.LowerRightCorner.Y;
        if (left > rect.UpperLeftCorner.X)
        {
            rect.LowerRightCorner.X = left;
            clipAgainst(rect, AbsoluteClippingRect);
            for (int i = rect.UpperLeftCorner.X; i < rect.LowerRightCorner.X; i += width)
                Animations[ETBRA_TOP]->draw(ox::core::CPosition2d<int>(i, top), &rect, white);
        }

        rect = ox::core::CRect<int>(right, frameRect.UpperLeftCorner.Y, frameRect.LowerRightCorner.X,
            frameRect.LowerRightCorner.Y);
        clipAgainst(rect, AbsoluteClippingRect);
        for (int i = rect.UpperLeftCorner.X; i < rect.LowerRightCorner.X; i += width)
            Animations[ETBRA_TOP]->draw(ox::core::CPosition2d<int>(i, top), &rect, white);

        // the inner top edge below the row, clipped by the parent
        if (Animations[ETBRA_TOP_INNER])
        {
            rect = ox::core::CRect<int>(frameRect.UpperLeftCorner.X, frameRect.LowerRightCorner.Y,
                frameRect.LowerRightCorner.X, Rects[ETBRA_TOP_INNER].LowerRightCorner.Y +
                    frameRect.LowerRightCorner.Y - Rects[ETBRA_TOP_INNER].UpperLeftCorner.Y);
            clipAgainst(rect, Parent->getAbsolutePosition());
            int step = Rects[ETBRA_TOP_INNER].getWidth();
            for (int i = rect.UpperLeftCorner.X; i < rect.LowerRightCorner.X; i += step)
                Animations[ETBRA_TOP_INNER]->draw(ox::core::CPosition2d<int>(i, rect.UpperLeftCorner.Y), &rect,
                    white);
        }
    }
}

void CGUITabButtonRow::setAnimations(ox::video::ISpritePackage* package, const char* name)
{
    const char* TABBUTTONROW_ANIMATION_NAMES[ETBRA_COUNT] = { "Top", "TabNormal", "TabHighlighted", "TopInner",
        "TabCloseBtnNormal", "TabCloseBtnHiglighted" };

    for (int i = 0; i < ETBRA_COUNT; ++i)
    {
        if (Animations[i])
        {
            Animations[i]->remove();
            Animations[i] = 0;
        }

        ox::core::CString<char> animation = name;
        animation += ox::core::CString<char>(TABBUTTONROW_ANIMATION_NAMES[i]);
        // the close buttons are shared with the windows
        if (i == ETBRA_CLOSE_HIGHLIGHTED)
            animation = "CloseBtnHighlighted";
        else if (i == ETBRA_CLOSE_NORMAL)
            animation = "CloseBtnNormal";

        Animations[i] = package->addNewAnimationState(animation.c_str());
        if (Animations[i])
        {
            ox::core::CDimension2d<int> size = Animations[i]->getFrameSize(0);
            Rects[i] = ox::core::CRect<int>(0, 0, size.Width, size.Height);
        }
    }

    ClosePosition.X = Rects[ETBRA_TAB_NORMAL].getWidth() - 2 - Rects[ETBRA_CLOSE_NORMAL].getWidth();
    ClosePosition.Y = (Rects[ETBRA_TAB_NORMAL].getHeight() - Rects[ETBRA_CLOSE_NORMAL].getHeight()) / 2;

    setRelativePosition(ox::core::CRect<int>(RelativeRect.UpperLeftCorner.X, RelativeRect.UpperLeftCorner.Y,
        RelativeRect.LowerRightCorner.X, RelativeRect.UpperLeftCorner.Y + Rects[ETBRA_TAB_NORMAL].getHeight()));
}

void CGUITabButtonRow::setTextColor(ox::video::SColor color)
{
    TextColor = color;
}

void CGUITabButtonRow::setTextFont(ox::gui::IGUIFont* font)
{
    TextFont = font;
}

void CGUITabButtonRow::setRelativePosition(const ox::core::CRect<int>& position)
{
    IGUIElement::setRelativePosition(position);
    repositionTabs();
}

//! destructor
CGUITabControl::~CGUITabControl()
{
    for (int i = 0; i < (int)Tabs.size(); ++i)
        if (Tabs[i])
            Tabs[i]->drop();

    for (int i = 0; i < ETCA_COUNT; ++i)
        if (Animations[i])
            Animations[i]->remove();
}

} // end namespace gui
} // end namespace daisy
