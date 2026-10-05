// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CGUIPopupMenu.h"
#include "ox/gui/IGUIElementInline.h"
#include "ox/algo/CArrayFunctions.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUISkin.h"
#include "ox/video/IVideoDriver.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

//! The height of the frame before the options sort it.
static const int INITIAL_FRAME_HEIGHT = 40;

CGUIPopupMenu::CGUIPopupMenu(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
    ox::core::CPosition2d<int> position, int width, const wchar_t* text, ox::gui::IGUIFont* titleFont,
    ox::gui::IGUIFont* font)
    : IGUIPopupMenu(environment, parent, id, ox::core::CRect<int>(ox::core::CPosition2d<int>(0, 0), parent->getRelativePosition().LowerRightCorner)),
      Selected(-1), Width(width), Title(0), Font(font), SelectionParent(parent)
{
    Frame = environment->addFrame(ox::core::CRect<int>(position,
        ox::core::CDimension2d<int>(width, INITIAL_FRAME_HEIGHT)), this, -1);

    if (titleFont && text)
    {
        ox::core::CDimension2d<int> dim = titleFont->getDimension(text);
        Title = environment->addStaticText(text, ox::core::CRect<int>(0, 0, Width, dim.Height), false, false,
            Frame, -1, L"");
        Title->setOverrideFont(titleFont);
    }

    Frame->sortVertically(1, true);
}

CGUIPopupMenu::~CGUIPopupMenu()
{
}

void CGUIPopupMenu::addMenuOption(const wchar_t* text, int id)
{
    if (Font && Frame)
    {
        ox::core::CDimension2d<int> dim = Font->getDimension(text);
        ox::gui::IGUIStaticText* option = Environment->addStaticText(text,
            ox::core::CRect<int>(0, 0, Width, dim.Height), false, false, Frame, id, L"");
        option->setOverrideFont(Font);
        Frame->sortVertically(1, true);
        Selected = -1;
    }
}

void CGUIPopupMenu::setSelectionParent(ox::gui::IGUIElement* parent)
{
    SelectionParent = parent;
}

//! called if an event happened.
bool CGUIPopupMenu::OnEvent(const ox::event::SEvent& event)
{
    if (!isEnabled())
        return false;

    if (event.EventType == ox::event::EET_MOUSE_INPUT_EVENT)
    {
        ox::core::CPosition2d<int> pos(event.MouseInput.X, event.MouseInput.Y);

        if (Frame->getAbsolutePosition().isPointInside(pos))
        {
            if (event.MouseInput.Event == ox::event::EMIE_MOUSE_MOVED)
            {
                // the first child of the frame is the title
                std::list<ox::gui::IGUIElement*>::const_iterator it =
                    ox::algo::advanceIterator(Frame->getChildren().begin(), 1);
                for (int index = 0; it != Frame->getChildren().end(); ++it, ++index)
                {
                    if ((*it)->getAbsolutePosition().isPointInside(pos))
                    {
                        Selected = index;
                        return true;
                    }
                }
            }
            else if (event.MouseInput.Event == ox::event::EMIE_LMOUSE_LEFT_UP && Selected >= 0)
            {
                std::list<ox::gui::IGUIElement*>::const_iterator it =
                    ox::algo::advanceIterator(Frame->getChildren().begin(), 1);
                for (int index = 0; it != Frame->getChildren().end(); ++it, ++index)
                {
                    if (index == Selected)
                    {
                        ox::event::SEvent chosen;
                        chosen.EventType = ox::event::EET_GUI_EVENT;
                        chosen.GUIEvent.Caller = *it;
                        chosen.GUIEvent.EventType = ox::gui::EGET_POPUP_MENU_OPTION_CHOSEN;
                        if (SelectionParent)
                            SelectionParent->OnEvent(chosen);
                        else if (Parent)
                            Parent->OnEvent(chosen);
                        break;
                    }
                }

                ox::event::SEvent closed;
                closed.EventType = ox::event::EET_GUI_EVENT;
                closed.GUIEvent.Caller = this;
                closed.GUIEvent.EventType = ox::gui::EGET_POPUP_MENU_CLOSED;
                if (SelectionParent)
                    SelectionParent->OnEvent(closed);
                else if (Parent)
                    Parent->OnEvent(closed);
                remove();
                return true;
            }
            return true;
        }

        // a press outside the frame closes the menu
        if ((unsigned int)event.MouseInput.Event <= ox::event::EMIE_MMOUSE_PRESSED_DOWN)
        {
            ox::event::SEvent closed;
            closed.EventType = ox::event::EET_GUI_EVENT;
            closed.GUIEvent.Caller = this;
            closed.GUIEvent.EventType = ox::gui::EGET_POPUP_MENU_CLOSED;
            if (SelectionParent)
                SelectionParent->OnEvent(closed);
            else if (Parent)
                Parent->OnEvent(closed);
            remove();
            return false;
        }

        Selected = -1;
    }

    return Parent ? Parent->OnEvent(event) : false;
}

//! draws the element and frames the selected option
void CGUIPopupMenu::draw()
{
    if (!IsVisible)
        return;

    IGUIElement::draw();

    if (Selected < 0 || Selected >= (int)(Frame->getChildren().size() - 1))
        return;

    std::list<ox::gui::IGUIElement*>::const_iterator it = Frame->getChildren().begin();
    ox::core::CRect<int> rect = (*ox::algo::advanceIterator(it, Selected + 1))->getAbsolutePosition();
    ox::video::IVideoDriver* driver = Environment->getVideoDriver();
    ox::video::SColor fill = Environment->getSkin()->getColor(ox::gui::EGDC_POPUP_MENU_HIGHLIGHT);
    ox::video::SColor border = Environment->getSkin()->getColor(ox::gui::EGDC_POPUP_MENU_HIGHLIGHT_BORDER);

    driver->draw2DRectangle(fill, rect, &AbsoluteRect);
    driver->draw2DRectangle(border, ox::core::CRect<int>(rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y,
        rect.UpperLeftCorner.X + 2, rect.LowerRightCorner.Y), &AbsoluteRect);
    driver->draw2DRectangle(border, ox::core::CRect<int>(rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y,
        rect.LowerRightCorner.X, rect.UpperLeftCorner.Y + 2), &AbsoluteRect);
    driver->draw2DRectangle(border, ox::core::CRect<int>(rect.UpperLeftCorner.X, rect.LowerRightCorner.Y - 2,
        rect.LowerRightCorner.X, rect.LowerRightCorner.Y), &AbsoluteRect);
    driver->draw2DRectangle(border, ox::core::CRect<int>(rect.LowerRightCorner.X - 2, rect.UpperLeftCorner.Y,
        rect.LowerRightCorner.X, rect.LowerRightCorner.Y), &AbsoluteRect);
}

} // end namespace gui
} // end namespace daisy
