// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIMessageBox.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye puts the buttons in a layout group, sorts the box vertically and answers return and escape.

#include "CGUIMessageBox.h"
#include "ox/gui/IGUIElementInline.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUISkin.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

//! The spacing of the buttons and of the rows of the box.
static const int ROW_SPACING = 5;
//! The width the button row starts with before it is sorted.
static const int BUTTON_ROW_WIDTH = 50;

//! constructor
CGUIMessageBox::CGUIMessageBox(ox::gui::IGUIEnvironment* environment, const wchar_t* caption, const wchar_t* text,
    int flags, ox::gui::IGUIElement* parent, int id, ox::core::CRect<int> rectangle)
    : CGUIWindow(environment, parent, id, rectangle, false), OkButton(0), CancelButton(0), YesButton(0),
      NoButton(0), ButtonLayout(0), StaticText(0), KeyPressed(false)
{
    // remove focus
    Environment->setFocus(0);

    // remove buttons
    if (getMaximizeButton())
        getMaximizeButton()->remove();
    if (getMinimizeButton())
        getMinimizeButton()->remove();

    if (caption)
        setText(caption);

    ox::gui::IGUISkin* skin = Environment->getSkin();

    int buttonHeight = skin->getSize(ox::gui::EGDS_BUTTON_HEIGHT);
    int buttonWidth = skin->getSize(ox::gui::EGDS_BUTTON_WIDTH);
    int titleHeight = skin->getSize(ox::gui::EGDS_WINDOW_BUTTON_WIDTH) + 2;
    int buttonDistance = skin->getSize(ox::gui::EGDS_WINDOW_BUTTON_WIDTH);
    (void)buttonDistance;

    // add static multiline text
    ox::core::CDimension2d<int> dim(AbsoluteClippingRect.getWidth() - buttonWidth,
        AbsoluteClippingRect.getHeight() - (buttonHeight * 3));
    ox::core::CPosition2d<int> pos((AbsoluteClippingRect.getWidth() - dim.Width) / 2, buttonHeight / 2 + titleHeight);

    StaticText = Environment->addStaticText(text, ox::core::CRect<int>(pos, dim), false, false, this, -1, 0);
    StaticText->setWordWrap(true);
    StaticText->grab();

    // adjust static text height
    int textHeight = StaticText->getTextHeight();
    ox::core::CRect<int> tmp = StaticText->getRelativePosition();
    tmp.LowerRightCorner.Y = tmp.UpperLeftCorner.Y + textHeight;
    StaticText->setRelativePosition(tmp);
    dim.Height = textHeight;

    // adjust message box height
    tmp = getRelativePosition();
    int msgBoxHeight = textHeight + (int)(2.5f * buttonHeight) + titleHeight;

    // adjust message box position
    tmp.UpperLeftCorner.Y = (parent->getAbsolutePosition().getHeight() - msgBoxHeight) / 2;
    tmp.LowerRightCorner.Y = tmp.UpperLeftCorner.Y + msgBoxHeight;
    setRelativePosition(tmp);

    // add buttons
    ButtonLayout = Environment->addLayoutGroup(ox::core::CRect<int>(0, 0, BUTTON_ROW_WIDTH, buttonHeight + 2), this);
    ButtonLayout->grab();

    ox::core::CRect<int> btnRect(0, 0, buttonWidth, buttonHeight);

    if (flags & ox::gui::EMBF_NO)
    {
        NoButton = Environment->addButton(btnRect, ButtonLayout, -1, skin->getDefaultText(ox::gui::EGDT_MSG_BOX_NO));
        NoButton->grab();
    }

    if (flags & ox::gui::EMBF_CANCEL)
    {
        CancelButton = Environment->addButton(btnRect, ButtonLayout, -1,
            skin->getDefaultText(ox::gui::EGDT_MSG_BOX_CANCEL));
        CancelButton->grab();
    }

    if (flags & ox::gui::EMBF_OK)
    {
        OkButton = Environment->addButton(btnRect, ButtonLayout, -1, skin->getDefaultText(ox::gui::EGDT_MSG_BOX_OK));
        OkButton->grab();
        Environment->setFocus(OkButton);
    }

    if (flags & ox::gui::EMBF_YES)
    {
        YesButton = Environment->addButton(btnRect, ButtonLayout, -1, skin->getDefaultText(ox::gui::EGDT_MSG_BOX_YES));
        YesButton->grab();
    }

    ButtonLayout->sortHorizontally(ROW_SPACING);
    sortVertically(ROW_SPACING, true);
}

//! destructor
CGUIMessageBox::~CGUIMessageBox()
{
    if (StaticText)
        StaticText->drop();

    if (OkButton)
        OkButton->drop();

    if (CancelButton)
        CancelButton->drop();

    if (YesButton)
        YesButton->drop();

    if (NoButton)
        NoButton->drop();

    if (ButtonLayout)
        ButtonLayout->drop();
}

//! called if an event happened.
bool CGUIMessageBox::OnEvent(const ox::event::SEvent& event)
{
    ox::event::SEvent outevent;
    outevent.EventType = ox::event::EET_GUI_EVENT;
    outevent.GUIEvent.Caller = this;

    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
        if (event.GUIEvent.EventType == ox::gui::EGET_BUTTON_CLICKED)
        {
            if (event.GUIEvent.Caller == OkButton)
            {
                outevent.GUIEvent.EventType = ox::gui::EGET_MESSAGEBOX_OK;
                Parent->OnEvent(outevent);
                remove();
                return true;
            }
            else if (event.GUIEvent.Caller == CancelButton || event.GUIEvent.Caller == CloseButton)
            {
                outevent.GUIEvent.EventType = ox::gui::EGET_MESSAGEBOX_CANCEL;
                Parent->OnEvent(outevent);
                remove();
                return true;
            }
            else if (event.GUIEvent.Caller == YesButton)
            {
                outevent.GUIEvent.EventType = ox::gui::EGET_MESSAGEBOX_YES;
                Parent->OnEvent(outevent);
                remove();
                return true;
            }
            else if (event.GUIEvent.Caller == NoButton)
            {
                outevent.GUIEvent.EventType = ox::gui::EGET_MESSAGEBOX_NO;
                Parent->OnEvent(outevent);
                remove();
                return true;
            }
        }
        break;
    default:
        break;
    }

    return CGUIWindow::OnEvent(event);
}

bool CGUIMessageBox::OnEventInNonFocusState(const ox::event::SEvent& event)
{
    ox::event::SEvent outevent;
    outevent.EventType = ox::event::EET_GUI_EVENT;
    outevent.GUIEvent.Caller = this;

    if (event.EventType == ox::event::EET_KEY_INPUT_EVENT)
    {
        if (event.KeyInput.Event == ox::event::EKIE_KEY_PRESSED_DOWN)
        {
            if (event.KeyInput.Key == ox::KEY_RETURN || event.KeyInput.Key == ox::KEY_ESCAPE)
            {
                KeyPressed = true;
                return true;
            }
        }
        else if (event.KeyInput.Event == ox::event::EKIE_KEY_LEFT_UP && KeyPressed)
        {
            if (event.KeyInput.Key == ox::KEY_RETURN)
            {
                if (YesButton)
                {
                    outevent.GUIEvent.EventType = ox::gui::EGET_MESSAGEBOX_YES;
                    Parent->OnEvent(outevent);
                    remove();
                    return true;
                }
                if (OkButton)
                {
                    outevent.GUIEvent.EventType = ox::gui::EGET_MESSAGEBOX_OK;
                    Parent->OnEvent(outevent);
                    remove();
                    return true;
                }
                return true;
            }
            else if (event.KeyInput.Key == ox::KEY_ESCAPE)
            {
                if (CancelButton)
                {
                    outevent.GUIEvent.EventType = ox::gui::EGET_MESSAGEBOX_CANCEL;
                    Parent->OnEvent(outevent);
                    remove();
                    return true;
                }
                if (NoButton)
                {
                    outevent.GUIEvent.EventType = ox::gui::EGET_MESSAGEBOX_NO;
                    Parent->OnEvent(outevent);
                    remove();
                    return true;
                }
                return true;
            }
        }
    }

    return false;
}

} // end namespace gui
} // end namespace daisy
