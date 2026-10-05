// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIElement.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source.
// Inline bodies of the IGUIElement constructor and virtual methods, copied from the Linux build's COMDATs.
// They live apart from IGUIElement.h because more inline functions there change the code GCC
// generates for game units that include it; the daisy::gui widgets include this header.

#ifndef OX_GUI_IGUIELEMENTINLINES_H
#define OX_GUI_IGUIELEMENTINLINES_H

#include "IGUIElement.h"

namespace ox {
namespace gui {

inline IGUIElement::IGUIElement(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
    : Parent(parent), RelativeRect(rectangle), RelativeSizeChanged(false), IsVisible(true), IsEnabled(true),
      IsFixed(false), IsInvisible(false), NoClip(false), ReportOnDraw(0), ID(id), Type(0),
      Environment(environment), HoverItem(0), LayoutFlags(0), EventParent(0)
{
    AbsoluteRect = RelativeRect;
    AbsoluteClippingRect = RelativeRect;
    updateAbsolutePosition();

    if (Parent)
        Parent->addChild(this);
}

//! Draws the children; elements outside their clipping rectangle are skipped when fixed.
inline void IGUIElement::draw()
{
    if (!IsVisible)
        return;

    if (ReportOnDraw == 1)
    {
        event::SEvent e;
        e.EventType = event::EET_GUI_EVENT;
        e.GUIEvent.Caller = this;
        e.GUIEvent.EventType = EGET_ELEMENT_DRAWN;
        OnEvent(e);
    }

    for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
    {
        if ((*it)->AbsoluteRect.isRectCollided((*it)->AbsoluteClippingRect) || !(*it)->isFixed())
            (*it)->draw();
    }

    if (ReportOnDraw == 2)
    {
        event::SEvent e;
        e.EventType = event::EET_GUI_EVENT;
        e.GUIEvent.Caller = this;
        e.GUIEvent.EventType = EGET_ELEMENT_DRAWN;
        OnEvent(e);
    }
}

inline void IGUIElement::move(core::CPosition2d<int> offset)
{
    RelativeRect.UpperLeftCorner.X += offset.X;
    RelativeRect.UpperLeftCorner.Y += offset.Y;
    RelativeRect.LowerRightCorner.X += offset.X;
    RelativeRect.LowerRightCorner.Y += offset.Y;
    updateAbsolutePosition();
}

inline void IGUIElement::moveTo(core::CPosition2d<int> position)
{
    setRelativePosition(core::CRect<int>(position.X, position.Y, position.X + RelativeRect.getWidth(),
        position.Y + RelativeRect.getHeight()));
}

//! Moves the element to the center of a rectangle of the parent's size.
inline void IGUIElement::centerOnRect(const core::CRect<int>& rect)
{
    moveTo(core::CPosition2d<int>((rect.getWidth() - RelativeRect.getWidth()) / 2,
        (rect.getHeight() - RelativeRect.getHeight()) / 2));
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

//! Offers the event to the event parent, then passes it to the parent.
inline bool IGUIElement::OnEvent(const event::SEvent& event)
{
    if (EventParent && EventParent->OnEvent(event))
        return true;

    if (Parent)
        return Parent->OnEvent(event);

    return true;
}

inline bool IGUIElement::OnEventInNonFocusState(const event::SEvent& event)
{
    for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
        if ((*it)->OnEventInNonFocusState(event))
            return true;

    return false;
}

inline bool IGUIElement::bringToFront(IGUIElement* element)
{
    for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
    {
        if (element == *it)
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
            return *it;

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

//! Sets the element shown while the mouse hovers over this one.
inline void IGUIElement::setHoverItem(IGUIElement* item)
{
    HoverItem = item;
    if (item)
    {
        item->setVisible(false);
        HoverItem->setFixed(true);
        HoverItem->NoClip = true;
        HoverItem->updateAbsolutePosition();
    }
}

inline core::CDimension2d<int> IGUIElement::getPreferredSize()
{
    return core::CDimension2d<int>(RelativeRect.getWidth(), RelativeRect.getHeight());
}

} // end namespace gui
} // end namespace ox

#endif
