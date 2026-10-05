// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CGUIRadioList.h"
#include "ox/gui/IGUIElementInline.h"
#include "CGUICheckBox.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUISkin.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

//! The first button's rectangle when the skin has no sprites.
static const int FIRST_BUTTON_X = 1;
static const int FIRST_BUTTON_WIDTH = 9;
//! The space a button keeps beside its sprite.
static const int BUTTON_MARGIN = 2;

CGUIRadioList::CGUIRadioList(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent,
    ox::core::CRect<int> rectangle, int id)
    : IGUIRadioList(environment, parent, id, rectangle), Selection(0)
{
    TextColor = Environment->getSkin()->getColor(ox::gui::EGDC_BUTTON_TEXT);
    IsInvisible = true;
}

CGUIRadioList::~CGUIRadioList()
{
    for (int i = 0; i < (int)Radios.size(); ++i)
        if (Radios[i])
            Radios[i]->drop();
}

bool CGUIRadioList::OnEvent(const ox::event::SEvent& event)
{
    if (event.EventType == ox::event::EET_GUI_EVENT &&
        event.GUIEvent.EventType == ox::gui::EGET_CHECKBOX_TOGGLED &&
        event.GUIEvent.Caller->getID() == -getID())
    {
        if (Radios[Selection]->isChecked())
        {
            // another button was checked
            Radios[Selection]->setChecked(false);
            for (int i = 0; i < (int)Radios.size(); ++i)
                if (Radios[i]->isChecked())
                    Selection = i;

            ox::event::SEvent newEvent;
            newEvent.EventType = ox::event::EET_GUI_EVENT;
            newEvent.GUIEvent.Caller = this;
            newEvent.GUIEvent.EventType = ox::gui::EGET_RADIOLIST_CHANGED;
            Parent->OnEvent(newEvent);
            return true;
        }

        // the checked button was clicked again
        Radios[Selection]->setChecked(true);
        return true;
    }

    return Parent ? Parent->OnEvent(event) : false;
}

ox::gui::IGUICheckBox* CGUIRadioList::addRadioButton(const wchar_t* text)
{
    ox::gui::IGUISkin* skin = Environment->getSkin();
    if (!skin)
        return 0;

    int height = skin->getSize(ox::gui::EGDS_RADIO_BUTTON_HEIGHT);
    int spacing = skin->getSize(ox::gui::EGDS_RADIO_BUTTON_SPACING);

    ox::core::CRect<int> rect;
    if (!Radios.empty())
    {
        // below the last button
        rect = Radios[Radios.size() - 1]->getRelativePosition();
        int step = rect.getHeight();
        step += skin->getSize(ox::gui::EGDS_RADIO_BUTTON_SPACING);
        rect.UpperLeftCorner.Y += step;
        rect.LowerRightCorner.Y += step;
    }
    else
        rect = ox::core::CRect<int>(FIRST_BUTTON_X, 0, FIRST_BUTTON_X + FIRST_BUTTON_WIDTH, height + spacing);

    CGUICheckBox* button = new CGUICheckBox(false, Environment, this, -getID(), rect);
    if (text)
        button->setText(text);

    if (Radios.empty())
    {
        button->setChecked(true);
        Selection = 0;
    }

    int width = 0;
    ox::video::ISpritePackage* package = Environment->getSkin()->getSpritePackage();
    if (package)
    {
        button->setAnimations(package, "Radiobutton");

        ox::core::CRect<int> buttonRect = button->getRelativePosition();
        if (buttonRect.getHeight() < skin->getSize(ox::gui::EGDS_RADIO_BUTTON_HEIGHT))
        {
            buttonRect.LowerRightCorner.Y = buttonRect.UpperLeftCorner.Y + skin->getSize(ox::gui::EGDS_RADIO_BUTTON_HEIGHT);
            button->setRelativePosition(buttonRect);
        }

        button->updateWidth();
        buttonRect = button->getRelativePosition();
        width = buttonRect.getWidth();
    }

    button->setTextColor(TextColor);
    Radios.push_back(button);

    // grow to fit the buttons
    ox::core::CRect<int> listRect = RelativeRect;
    listRect.LowerRightCorner.Y = RelativeRect.UpperLeftCorner.Y + rect.LowerRightCorner.Y;
    if (width + BUTTON_MARGIN > listRect.getWidth())
        listRect.LowerRightCorner.X = listRect.UpperLeftCorner.X + width + BUTTON_MARGIN;
    setRelativePosition(listRect);

    return button;
}

int CGUIRadioList::getRadioCount()
{
    return Radios.size();
}

ox::gui::IGUICheckBox* CGUIRadioList::getRadioButton(int index)
{
    if (index < 0 || index >= (int)Radios.size())
        return 0;
    return Radios[index];
}

void CGUIRadioList::checkRadioButton(int index)
{
    if (index < 0 || index >= (int)Radios.size())
        return;

    for (int i = 0; i < (int)Radios.size(); ++i)
        Radios[i]->setChecked(false);

    Radios[index]->setChecked(true);
    Selection = index;
}

int CGUIRadioList::getSelection()
{
    return Selection;
}

void CGUIRadioList::setTextColor(ox::video::SColor color)
{
    TextColor = color;
}

} // end namespace gui
} // end namespace daisy
