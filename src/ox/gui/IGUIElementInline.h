// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIElement.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Inline bodies of IGUIElement that the element implementations inline and emit. They live apart from
// IGUIElement.h because more inline functions there change the code GCC generates for game units
// that include it (CSaveGameScreen::loadSaveGames). The widget units need them all: their size
// decides which destructor calls GCC inlines into exception cleanups.

#ifndef OX_GUI_IGUIELEMENTINLINE_H
#define OX_GUI_IGUIELEMENTINLINE_H

#include "IGUIElement.h"

namespace ox {
namespace gui {

inline IGUIElement::IGUIElement(IGUIEnvironment* environment, IGUIElement* parent, int id,
    core::CRect<int> rectangle)
    : Parent(parent), RelativeRect(rectangle), RelativeSizeChanged(false), IsVisible(true), IsEnabled(true),
      IsFixed(false), IsInvisible(false), NoClip(false), ReportOnDraw(0), ID(id), Type(0),
      Environment(environment), HoverItem(0), LayoutFlags(0), EventReceiver(0)
{
    AbsoluteRect = RelativeRect;
    AbsoluteClippingRect = RelativeRect;
    updateAbsolutePosition();

    if (Parent)
        Parent->addChild(this);
}

//! Draws the visible children; ReportOnDraw 1 reports before and 2 after drawing them.
inline void IGUIElement::draw()
{
    if (!IsVisible)
        return;

    if (ReportOnDraw == 1)
    {
        event::SEvent event;
        event.EventType = event::EET_GUI_EVENT;
        event.GUIEvent.Caller = this;
        event.GUIEvent.EventType = EGET_ELEMENT_DRAWN;
        OnEvent(event);
    }

    for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
    {
        if ((*it)->AbsoluteRect.isRectCollided((*it)->AbsoluteClippingRect) || !(*it)->isFixed())
            (*it)->draw();
    }

    if (ReportOnDraw == 2)
    {
        event::SEvent event;
        event.EventType = event::EET_GUI_EVENT;
        event.GUIEvent.Caller = this;
        event.GUIEvent.EventType = EGET_ELEMENT_DRAWN;
        OnEvent(event);
    }
}

//! Moves the element by an offset.
inline void IGUIElement::move(core::CPosition2d<int> absoluteMovement)
{
    RelativeRect.UpperLeftCorner += absoluteMovement;
    RelativeRect.LowerRightCorner += absoluteMovement;
    updateAbsolutePosition();
}

//! Moves the element to a relative position, keeping its size.
inline void IGUIElement::moveTo(core::CPosition2d<int> position)
{
    setRelativePosition(core::CRect<int>(position.X, position.Y,
        RelativeRect.LowerRightCorner.X + position.X - RelativeRect.UpperLeftCorner.X,
        RelativeRect.LowerRightCorner.Y + position.Y - RelativeRect.UpperLeftCorner.Y));
}

//! Moves the element to the center of a rectangle of the parent's size.
inline void IGUIElement::centerOnRect(const core::CRect<int>& rect)
{
    core::CPosition2d<int> position((rect.getWidth() - RelativeRect.getWidth()) / 2,
        (rect.getHeight() - RelativeRect.getHeight()) / 2);
    moveTo(position);
}

inline void IGUIElement::centerOnParent()
{
    centerOnRect(Parent->getRelativePosition());
}

inline bool IGUIElement::isVisible()
{
    return IsVisible;
}

inline void IGUIElement::setVisible(bool visible)
{
    IsVisible = visible;
}

inline bool IGUIElement::isEnabled()
{
    return IsEnabled;
}

//! Enables or disables the element and its children.
inline void IGUIElement::setEnabled(bool enabled)
{
    IsEnabled = enabled;
    for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
        (*it)->setEnabled(enabled);
}

inline bool IGUIElement::isFixed()
{
    return IsFixed;
}

inline void IGUIElement::setFixed(bool fixed)
{
    IsFixed = fixed;
}

inline bool IGUIElement::isInvisible()
{
    return IsInvisible;
}

inline void IGUIElement::setInvisible(bool invisible)
{
    IsInvisible = invisible;
}

inline bool IGUIElement::doesReportOnDraw()
{
    return ReportOnDraw != 0;
}

inline void IGUIElement::setReportOnDraw(int report)
{
    ReportOnDraw = report;
}

inline void IGUIElement::setText(const wchar_t* text)
{
    Text = text;
}

inline const wchar_t* IGUIElement::getText() const
{
    return Text.c_str();
}

inline int IGUIElement::getID()
{
    return ID;
}

inline void IGUIElement::setID(int id)
{
    ID = id;
}

inline int IGUIElement::getType()
{
    return Type;
}

//! The event receiver sees events first; unhandled ones go to the parent.
inline bool IGUIElement::OnEvent(const event::SEvent& event)
{
    if (EventReceiver && EventReceiver->OnEvent(event))
        return true;

    if (Parent)
        return Parent->OnEvent(event);

    return true;
}

inline bool IGUIElement::OnEventInNonFocusState(const event::SEvent& event)
{
    for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
    {
        if ((*it)->OnEventInNonFocusState(event))
            return true;
    }
    return false;
}

//! Brings a child to the front of the drawing order.
inline bool IGUIElement::bringToFront(IGUIElement* element)
{
    for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
    {
        if (element == (*it))
        {
            Children.erase(it);
            Children.push_back(element);
            return true;
        }
    }
    return false;
}

inline const std::list<IGUIElement*>& IGUIElement::getChildren()
{
    return Children;
}

inline IGUIElement* IGUIElement::getElementFromId(int id, bool searchChildren)
{
    IGUIElement* e = 0;

    for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
    {
        if ((*it)->getID() == id)
            return (*it);

        if (searchChildren)
            e = (*it)->getElementFromId(id, true);

        if (e)
            return e;
    }

    return e;
}

inline IGUIElement* IGUIElement::getHoverItem()
{
    return HoverItem;
}

//! Sets the element shown when the mouse hovers over this one; it is hidden and unclipped.
inline void IGUIElement::setHoverItem(IGUIElement* item)
{
    HoverItem = item;
    if (HoverItem)
    {
        HoverItem->setVisible(false);
        HoverItem->setFixed(true);
        HoverItem->NoClip = true;
        HoverItem->updateAbsolutePosition();
    }
}

inline core::CDimension2d<int> IGUIElement::getPreferredSize()
{
    return core::CDimension2d<int>(RelativeRect.getWidth(), RelativeRect.getHeight());
}

//! Returns the topmost visible element at the point, searching the children from back to front.
inline IGUIElement* IGUIElement::getElementFromPoint(const core::CPosition2d<int>& point)
{
    if (!AbsoluteClippingRect.isPointInside(point))
        return 0;

    IGUIElement* target = 0;
    if (IsVisible)
        for (std::list<IGUIElement*>::reverse_iterator it = Children.rbegin(); it != Children.rend(); ++it)
        {
            target = (*it)->getElementFromPoint(point);
            if (target)
                return target;
        }

    if (AbsoluteRect.isPointInside(point) && IsVisible)
        target = this;

    return target;
}

} // end namespace gui
} // end namespace ox

#endif
