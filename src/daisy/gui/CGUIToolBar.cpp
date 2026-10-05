// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIToolBar.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye ignores the button images.

#include "CGUIToolBar.h"
#include "ox/gui/IGUISkin.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/video/IVideoDriver.h"
#include "ox/gui/IGUIButton.h"
#include "ox/gui/IGUIFont.h"
#include "CGUIButton.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

//! constructor
CGUIToolBar::CGUIToolBar(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
    ox::core::CRect<int> rectangle)
    : IGUIToolBar(environment, parent, id, rectangle), ButtonX(5)
{
    // calculate position and find other menubars
    int y = 0;
    int parentwidth = 100;

    if (parent)
    {
        parentwidth = Parent->getAbsolutePosition().getWidth();

        const std::list<ox::gui::IGUIElement*>& children = parent->getChildren();
        std::list<ox::gui::IGUIElement*>::const_iterator it = children.begin();

        for (; it != children.end(); ++it)
        {
            ox::core::CRect<int> r = (*it)->getAbsolutePosition();
            if (r.UpperLeftCorner.X == 0 && r.UpperLeftCorner.Y <= y && r.LowerRightCorner.X == parentwidth)
                y = r.LowerRightCorner.Y;
        }
    }

    RelativeRect.UpperLeftCorner.X = 0;
    RelativeRect.UpperLeftCorner.Y = y;
    int height = 30;

    RelativeRect.LowerRightCorner.X = parentwidth;
    RelativeRect.LowerRightCorner.Y = RelativeRect.UpperLeftCorner.Y + height;
    updateAbsolutePosition();
}

//! destructor
CGUIToolBar::~CGUIToolBar()
{
}

//! draws the element and its children
void CGUIToolBar::draw()
{
    if (!IsVisible)
        return;

    ox::gui::IGUISkin* skin = Environment->getSkin();
    ox::gui::IGUIFont* font = skin->getFont();
    ox::gui::IGUIFont* defaultFont = Environment->getBuiltInFont();
    ox::video::IVideoDriver* driver = Environment->getVideoDriver();

    ox::core::CRect<int> rect = AbsoluteRect;
    ox::core::CRect<int>* clip = 0;

    rect.UpperLeftCorner.X = AbsoluteRect.UpperLeftCorner.X;
    rect.UpperLeftCorner.Y = AbsoluteRect.LowerRightCorner.Y - 1;
    rect.LowerRightCorner.Y = AbsoluteRect.LowerRightCorner.Y;
    rect.LowerRightCorner.X = AbsoluteRect.LowerRightCorner.X;
    driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_SHADOW), rect, clip);

    rect = AbsoluteRect;
    rect.LowerRightCorner.Y -= 1;
    driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_FACE), rect, clip);

    IGUIElement::draw();
}

//! Updates the absolute position.
void CGUIToolBar::updateAbsolutePosition()
{
    if (Parent)
        RelativeRect.LowerRightCorner.X = Parent->getAbsolutePosition().getWidth();

    IGUIElement::updateAbsolutePosition();
}

//! Adds a button to the tool bar
ox::gui::IGUIButton* CGUIToolBar::addButton(int id, const wchar_t* text, ox::video::ITexture* img,
    ox::video::ITexture* pressed, bool isPushButton)
{
    ButtonX += 3;

    ox::core::CRect<int> rectangle(ButtonX, 2, 0, 0);
    rectangle.LowerRightCorner.X = rectangle.UpperLeftCorner.X + 23;
    rectangle.LowerRightCorner.Y = rectangle.UpperLeftCorner.Y + 22;

    ButtonX += rectangle.getWidth();

    ox::gui::IGUIButton* button = new CGUIButton(Environment, this, id, rectangle);
    button->drop();

    if (text)
        button->setText(text);

    if (isPushButton)
        button->setIsPushButton(isPushButton);

    return button;
}

} // end namespace gui
} // end namespace daisy
