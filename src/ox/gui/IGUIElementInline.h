// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIElement.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source.
// The inline IGUIElement methods after remove(). The Linux build emits their copies in
// daisy/gui/CGUIEnvironment.cpp, the first unit whose vtables need them. They are kept out of
// IGUIElement.h because instantiating their templates there changes the register choices of the
// game units that include it.

#ifndef OX_GUI_IGUIELEMENTINLINE_H
#define OX_GUI_IGUIELEMENTINLINE_H

#include "IGUIElement.h"

namespace ox {
namespace gui {

inline IGUIElement::IGUIElement(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
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
        // children outside their clipping rectangle are skipped only when fixed
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
    RelativeRect.UpperLeftCorner += offset;
    RelativeRect.LowerRightCorner += offset;
    updateAbsolutePosition();
}

inline void IGUIElement::moveTo(core::CPosition2d<int> position)
{
    setRelativePosition(core::CRect<int>(position.X, position.Y, RelativeRect.getWidth() + position.X,
        RelativeRect.getHeight() + position.Y));
}

inline void IGUIElement::centerOnRect(const core::CRect<int>& rect)
{
    int x = (rect.getWidth() - RelativeRect.getWidth()) / 2;
    int y = (rect.getHeight() - RelativeRect.getHeight()) / 2;
    moveTo(core::CPosition2d<int>(x, y));
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

inline IGUIElement* IGUIElement::getElementFromPoint(const core::CPosition2d<int>& point)
{
    if (!AbsoluteClippingRect.isPointInside(point))
        return 0;

    IGUIElement* target = 0;
    if (IsVisible)
    {
        for (std::list<IGUIElement*>::reverse_iterator it = Children.rbegin(); it != Children.rend(); ++it)
        {
            target = (*it)->getElementFromPoint(point);
            if (target)
                return target;
        }
    }

    if (AbsoluteRect.isPointInside(point) && IsVisible)
        target = this;

    return target;
}

} // end namespace gui
} // end namespace ox

#endif
