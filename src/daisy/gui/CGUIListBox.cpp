// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIListBox.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Oxeye's list box holds arbitrary elements in a layout group that a scroll bar moves; text items
// are static texts. Selection events go to an override action parent instead of the parent.

#include "CGUIListBox.h"
#include "CGUIScrollBar.h"
#include "ox/algo/CArrayFunctions.h"
#include "ox/gui/IGUIElementInline.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/gui/IGUILayout.h"
#include "ox/gui/IGUIScrollBar.h"
#include "ox/gui/IGUISkin.h"
#include "ox/gui/IGUIStaticText.h"
#include "ox/video/IVideoDriver.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

//! Skin colors of the selection highlight; Oxeye's additions after Irrlicht's EGDC_COUNT.
static const ox::gui::EGUI_DEFAULT_COLOR SELECTION_COLOR = (ox::gui::EGUI_DEFAULT_COLOR)18;
static const ox::gui::EGUI_DEFAULT_COLOR SELECTION_BORDER_COLOR = (ox::gui::EGUI_DEFAULT_COLOR)19;

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

//! constructor
CGUIListBox::CGUIListBox(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
    ox::core::CRect<int> rectangle, bool clip, bool drawBack, bool moveOverSelect)
    : IGUIListBox(environment, parent, id, rectangle), Selected(-1), LastSelected(-1), ItemHeight(0),
      TotalItemHeight(0), Font(0), IconFont(0), Selecting(false), ItemSpacing(0), TextItemIndent(L"   "),
      ScrollBar(0), Frame(0), ListParent(0), Clip(clip), DrawBack(drawBack), MoveOverSelect(moveOverSelect),
      Selectable(false), SelectedAgain(false), OverrideActionParent(parent)
{
    Type = ox::gui::EGUIET_LIST_BOX;
    setInvisible(true);

    ox::core::CRect<int> rect(0, 0, RelativeRect.getWidth(), RelativeRect.getHeight());
    if (DrawBack)
        Frame = Environment->addFrame(rect, this, -1);
    else
    {
        Frame = Environment->addLayoutGroup(rect, this);
        Frame->setInvisible(false);
    }

    ox::core::CRect<int> area = Frame->getContentArea();
    ox::core::CPosition2d<int> offset(-area.UpperLeftCorner.X, -area.UpperLeftCorner.Y);
    area.UpperLeftCorner += offset;
    area.LowerRightCorner += offset;
    Frame->setRelativePosition(area);

    int scrollBarWidth = Environment->getSkin()->getSize(ox::gui::EGDS_SCROLLBAR_SIZE);
    area = Frame->getContentArea();

    ox::core::CRect<int> listRect(0, 0, area.getWidth() - 4 - scrollBarWidth, area.getHeight() - 4);
    ListParent = Environment->addLayoutGroup(listRect, Frame);
    listRect.LowerRightCorner.X = scrollBarWidth;
    ScrollBar = Environment->addScrollBar(false, listRect, Frame, -1);

    Frame->sortHorizontally(0);
    ScrollBar->setPos(0);
    ((CGUIScrollBar*)ScrollBar)->setListBoxParent(this);

    ListParentY = ListParent->getRelativePosition().UpperLeftCorner.Y;
    recalculateItemHeight();
    ScrollBarClicked = false;
}

void CGUIListBox::setRelativePosition(const ox::core::CRect<int>& position)
{
    IGUIElement::setRelativePosition(position);

    if (!Frame)
        return;

    Frame->setRelativePosition(ox::core::CRect<int>(0, 0, position.getWidth(), position.getHeight()));
    ox::core::CRect<int> area = Frame->getContentArea();
    int scrollBarWidth = Environment->getSkin()->getSize(ox::gui::EGDS_SCROLLBAR_SIZE);

    ox::core::CRect<int> listRect(0, 0, area.getWidth() - 4 - scrollBarWidth, area.getHeight() - 4);
    ListParent->setRelativePosition(listRect);
    listRect.LowerRightCorner.X = scrollBarWidth;
    ScrollBar->setRelativePosition(listRect);

    Frame->sortHorizontally(0);
    sortItems(false);
}

//! destructor
CGUIListBox::~CGUIListBox()
{
    if (Font)
        Font->drop();

    if (IconFont)
        IconFont->drop();
}

//! returns amount of list items
int CGUIListBox::getItemCount()
{
    return ListParent->getChildren().size();
}

//! returns the list item at an index
ox::gui::IGUIElement* CGUIListBox::getListItem(int index)
{
    return *ox::algo::advanceIterator(ListParent->getChildren().begin(), index);
}

//! returns the element holding the items
ox::gui::IGUIElement* CGUIListBox::getListParent()
{
    return ListParent;
}

//! stacks the items vertically; scrollToEnd shows the last items unless the scroll bar is dragged
void CGUIListBox::sortItems(bool scrollToEnd)
{
    ListParent->sortVertically(ItemSpacing, false);
    recalculateItemHeight();

    if (scrollToEnd)
    {
        if (ScrollBar->getMax() > 0)
        {
            if (!ScrollBar->isDragging())
                ScrollBar->setPos(ScrollBar->getMax());
        }
    }
}

//! clears the list
void CGUIListBox::clear()
{
    while (!ListParent->getChildren().empty())
        ListParent->removeChild(getListItem(0));

    TotalItemHeight = 0;
    Selected = -1;
    LastSelected = -1;

    if (ScrollBar)
        ScrollBar->setPos(0);

    recalculateItemHeight();
}

//! updates the item height and the scroll bar range from the list size
void CGUIListBox::recalculateItemHeight()
{
    ItemHeight = ListParent->getRelativePosition().getHeight();
    ScrollBar->setMax(ItemHeight + 2 - Frame->getContentArea().getHeight());

    if (!ListParent->getChildren().empty())
    {
        int step = ItemHeight / (int)ListParent->getChildren().size();
        ScrollBar->setStepSizes(step, step * 5);
    }
}

//! returns id of selected item. returns -1 if no item is selected.
int CGUIListBox::getSelected()
{
    return Selected;
}

//! sets the selected item. Set this to -1 if no item should be selected
void CGUIListBox::setSelected(int index)
{
    if (!Selectable || index < 0 || index > (int)ListParent->getChildren().size() - 1)
        Selected = -1;
    else
        Selected = index;

    LastSelected = Selected;
}

//! called if an event happened.
bool CGUIListBox::OnEvent(const ox::event::SEvent& event)
{
    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
        switch (event.GUIEvent.EventType)
        {
        case ox::gui::EGET_SCROLL_BAR_CHANGED:
            if (event.GUIEvent.Caller == ScrollBar)
            {
                ((ox::gui::IGUIScrollBar*)event.GUIEvent.Caller)->getPos();
                return true;
            }
            break;

        case ox::gui::EGET_ELEMENT_FOCUS_LOST:
            Selecting = false;
            return true;

        default:
            break;
        }
        break;

    case ox::event::EET_MOUSE_INPUT_EVENT:
        {
            ox::core::CPosition2d<int> p(event.MouseInput.X, event.MouseInput.Y);
            switch (event.MouseInput.Event)
            {
            case ox::event::EMIE_LMOUSE_PRESSED_DOWN:
                if (Environment->hasFocus(this) && ScrollBar->getAbsolutePosition().isPointInside(p) &&
                    ScrollBar->OnEvent(event))
                {
                    ScrollBarClicked = true;
                    return true;
                }

                if (getAbsolutePosition().isPointInside(p) && Selectable)
                {
                    Selecting = true;
                    Environment->setFocus(this);
                    return true;
                }
                break;

            case ox::event::EMIE_LMOUSE_LEFT_UP:
                if (Selectable)
                {
                    Selecting = false;
                    if (OverrideActionParent == Parent)
                        Environment->removeFocus(this);
                    selectNew(event.MouseInput.X, event.MouseInput.Y, false, event.MouseInput.Clicks);
                    return true;
                }
                break;

            case ox::event::EMIE_MOUSE_MOVED:
                if ((Selecting || MoveOverSelect) && getAbsolutePosition().isPointInside(p))
                {
                    selectNew(p.X, p.Y, true, 0);
                    return true;
                }
                break;

            case ox::event::EMIE_MOUSE_WHEEL:
                if (AbsoluteRect.isPointInside(p))
                {
                    ScrollBar->setPos(ScrollBar->getPos() + (int)event.MouseInput.ScrollY * -10);
                    return true;
                }
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }

    return OverrideActionParent ? OverrideActionParent->OnEvent(event) : false;
}

//! selects the item under a point and reports it unless onlyHover is set
bool CGUIListBox::selectNew(int x, int y, bool onlyHover, int clicks)
{
    if (!Selectable)
        return false;

    ox::gui::IGUIElement* item = ListParent->getElementFromPoint(ox::core::CPosition2d<int>(x, y));
    if (!item || item == ListParent)
        return false;

    // find the item that is a direct child of the list parent
    for (; item; item = item->getParent())
    {
        if (item->getParent() == getListParent())
            break;
    }

    const std::list<ox::gui::IGUIElement*>& children = ListParent->getChildren();
    int index = 0;
    for (std::list<ox::gui::IGUIElement*>::const_iterator it = children.begin(); it != children.end(); ++it)
    {
        if (item == *it)
            break;
        ++index;
    }
    Selected = index;

    if (!OverrideActionParent || onlyHover)
        return false;

    ox::event::SEvent event;
    event.EventType = ox::event::EET_GUI_EVENT;
    event.GUIEvent.Caller = this;
    if (LastSelected == index)
    {
        SelectedAgain = clicks > 1;
        event.GUIEvent.EventType = ox::gui::EGET_LISTBOX_SELECTED_AGAIN;
    }
    else
    {
        SelectedAgain = false;
        event.GUIEvent.EventType = ox::gui::EGET_LISTBOX_CHANGED;
    }
    LastSelected = index;
    OverrideActionParent->OnEvent(event);
    return true;
}

void CGUIListBox::setSelectable(bool selectable)
{
    Selectable = selectable;

    if (Selectable && Selected < 0 && !ListParent->getChildren().empty())
        Selected = 0;
    else if (!Selectable)
        Selected = -1;
}

void CGUIListBox::removeItem(int index)
{
    const std::list<ox::gui::IGUIElement*>& children = ListParent->getChildren();
    int i = 0;
    for (std::list<ox::gui::IGUIElement*>::const_iterator it = children.begin(); it != children.end(); ++it, ++i)
    {
        if (i == index)
        {
            ListParent->removeChild(*it);
            break;
        }
    }

    sortItems(false);
}

//! returns if the last selection selected the selected item again with a double click
bool CGUIListBox::selectionWasDoubleClicked()
{
    return SelectedAgain;
}

ox::gui::IGUIElement* CGUIListBox::getScrollBar()
{
    return ScrollBar;
}

//! draws the element and its children
void CGUIListBox::draw()
{
    if (!IsVisible)
        return;

    ListParent->moveTo(ox::core::CPosition2d<int>(0, ListParentY - ScrollBar->getPos()));

    if (DrawBack)
        IGUIElement::draw();
    else
    {
        if (ListParent && ScrollBar)
        {
            ListParent->draw();
            ScrollBar->draw();
        }
    }

    if (Selected < 0 || Selected >= (int)ListParent->getChildren().size())
        return;

    std::list<ox::gui::IGUIElement*>::const_iterator it = ListParent->getChildren().begin();
    ox::gui::IGUIElement* item = *ox::algo::advanceIterator(it, Selected);
    ox::core::CRect<int> rect = item->getAbsolutePosition();

    ox::video::IVideoDriver* driver = Environment->getVideoDriver();
    ox::video::SColor color = Environment->getSkin()->getColor(SELECTION_COLOR);
    ox::video::SColor borderColor = Environment->getSkin()->getColor(SELECTION_BORDER_COLOR);

    // clip against the list parent
    ox::core::CRect<int> clip = rect;
    clipAgainst(clip, ListParent->getAbsoluteClippingRect());

    if (!rect.isRectCollided(clip))
        return;

    driver->draw2DRectangle(color, rect, &clip);
    driver->draw2DRectangle(borderColor, ox::core::CRect<int>(rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y,
        rect.UpperLeftCorner.X + 2, rect.LowerRightCorner.Y), &clip);
    driver->draw2DRectangle(borderColor, ox::core::CRect<int>(rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y,
        rect.LowerRightCorner.X, rect.UpperLeftCorner.Y + 2), &clip);
    driver->draw2DRectangle(borderColor, ox::core::CRect<int>(rect.UpperLeftCorner.X, rect.LowerRightCorner.Y - 2,
        rect.LowerRightCorner.X, rect.LowerRightCorner.Y), &clip);
    driver->draw2DRectangle(borderColor, ox::core::CRect<int>(rect.LowerRightCorner.X - 2, rect.UpperLeftCorner.Y,
        rect.LowerRightCorner.X, rect.LowerRightCorner.Y), &clip);
}

void CGUIListBox::setIconFont(ox::gui::IGUIFont* font)
{
    if (IconFont)
        IconFont->drop();

    IconFont = font;

    if (IconFont)
        IconFont->grab();
}

//! adds a static text item; wordWrap breaks it into lines indented by the text item indent
ox::gui::IGUIElement* CGUIListBox::addTextItem(const wchar_t* text, ox::gui::IGUIFont* font,
    ox::video::SColor color, bool scrollToEnd, bool wordWrap)
{
    if (!font)
    {
        font = Environment->getSkin()->getFont();
        if (!font)
            return 0;
    }

    int width = ListParent->getContentArea().getWidth() - 8;
    int height = font->getDimension(L"A").Height;

    if (wordWrap)
        height = ox::gui::IGUIStaticText::getMultilineHeight(text, font, width, TextItemIndent.c_str());

    ox::gui::IGUIStaticText* item = Environment->addStaticText(text, ox::core::CRect<int>(0, 0, width, height),
        false, wordWrap, ListParent, -1, TextItemIndent.c_str());
    item->setOverrideColor(color);
    item->setOverrideFont(font);

    if (scrollToEnd)
        sortItems(true);

    return item;
}

//! sets the space between items
void CGUIListBox::setItemSpacing(int spacing)
{
    ItemSpacing = spacing;
}

//! sets the indent of the wrapped lines of text items
void CGUIListBox::setTextItemIndent(const wchar_t* indent)
{
    TextItemIndent = indent;
}

//! sets the element that receives the list events, the parent by default
void CGUIListBox::setOverrideActionParent(ox::gui::IGUIElement* parent)
{
    OverrideActionParent = parent;
}

} // end namespace gui
} // end namespace daisy
