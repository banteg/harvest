// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIElement.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. The virtual order follows the
// Mac 1.18 vtable of ox::gui::IGUIElement and the member offsets its accessors; member names and the
// flag types are inferred.

#ifndef OX_GUI_IGUIELEMENT_H
#define OX_GUI_IGUIELEMENT_H

#include "../IUnknown.h"
#include "../core/CDimension2d.h"
#include "../core/CPosition2d.h"
#include "../core/CRect.h"
#include "../core/CString.h"
#include <list>
#include "../event/IEventReceiver.h"

namespace ox {
namespace gui {

class IGUIEnvironment;

//! Element types returned by IGUIElement::getType. Partial: only the values recovered code checks.
enum EGUI_ELEMENT_TYPE
{
    EGUIET_SCROLL_BAR = 3,
    EGUIET_CHECK_BOX = 6,
    EGUIET_LIST_BOX = 8,
    EGUIET_COMBO_BOX = 16,
    EGUIET_BUTTON = 18,
    EGUIET_TEXT_BUTTON = 22,
    EGUIET_CLICK_AREA = 24
};

//! Base class of all GUI elements.
class IGUIElement : public IUnknown, public event::IEventReceiver
{
public:
    IGUIElement(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : Parent(parent), RelativeRect(rectangle), RelativeSizeChanged(false), IsVisible(true), IsEnabled(true),
          IsFixed(false), IsInvisible(false), NoClip(false), ReportOnDraw(0), ID(id), Type(0),
          Environment(environment), HoverItem(0), LayoutFlags(0), EventReceiver(0)
    {
        AbsoluteRect = RelativeRect;
        AbsoluteClippingRect = AbsoluteRect;
        updateAbsolutePosition();

        if (Parent)
            Parent->addChild(this);
    }

    // The methods up to remove() are inline, as in Irrlicht, so that remove() is the key function
    // and the vtable and destructors are emitted in IGUIElement.cpp, as in the Linux build.
    virtual ~IGUIElement()
    {
        for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
        {
            (*it)->Parent = 0;
            (*it)->drop();
        }
        Children.clear();

        if (HoverItem)
        {
            if (HoverItem->Parent)
                HoverItem->remove();
            else
                HoverItem->drop();
        }
    }

    virtual void setRelativePosition(const core::CRect<int>& position)
    {
        if (position.getHeight() != RelativeRect.getHeight() || position.getWidth() != RelativeRect.getWidth())
            RelativeSizeChanged = true;
        RelativeRect = position;
        updateAbsolutePosition();
    }

    virtual core::CRect<int> getParentAbsoluteClippingRect(bool clip)
    {
        if (IsInvisible && Parent)
            return Parent->getParentAbsoluteClippingRect(clip);
        return AbsoluteClippingRect;
    }

    virtual void updateAbsolutePosition()
    {
        core::CRect<int> parentAbsolute(0, 0, 0, 0);
        core::CRect<int> parentAbsoluteClip;
        if (Parent)
        {
            parentAbsolute = Parent->AbsoluteRect;
            parentAbsoluteClip = Parent->getParentAbsoluteClippingRect(IsFixed);
        }

        AbsoluteRect.UpperLeftCorner.X = RelativeRect.UpperLeftCorner.X + parentAbsolute.UpperLeftCorner.X;
        AbsoluteRect.UpperLeftCorner.Y = RelativeRect.UpperLeftCorner.Y + parentAbsolute.UpperLeftCorner.Y;
        AbsoluteRect.LowerRightCorner.X = RelativeRect.LowerRightCorner.X + parentAbsolute.UpperLeftCorner.X;
        AbsoluteRect.LowerRightCorner.Y = RelativeRect.LowerRightCorner.Y + parentAbsolute.UpperLeftCorner.Y;

        if (!Parent)
            parentAbsoluteClip = AbsoluteRect;

        AbsoluteClippingRect = AbsoluteRect;
        if (!NoClip)
        {
            // clip against the parent's clipping rectangle
            if (parentAbsoluteClip.LowerRightCorner.X < AbsoluteClippingRect.LowerRightCorner.X)
                AbsoluteClippingRect.LowerRightCorner.X = parentAbsoluteClip.LowerRightCorner.X;
            if (parentAbsoluteClip.LowerRightCorner.Y < AbsoluteClippingRect.LowerRightCorner.Y)
                AbsoluteClippingRect.LowerRightCorner.Y = parentAbsoluteClip.LowerRightCorner.Y;
            if (parentAbsoluteClip.UpperLeftCorner.X > AbsoluteClippingRect.UpperLeftCorner.X)
                AbsoluteClippingRect.UpperLeftCorner.X = parentAbsoluteClip.UpperLeftCorner.X;
            if (parentAbsoluteClip.UpperLeftCorner.Y > AbsoluteClippingRect.UpperLeftCorner.Y)
                AbsoluteClippingRect.UpperLeftCorner.Y = parentAbsoluteClip.UpperLeftCorner.Y;
        }

        for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
            (*it)->updateAbsolutePosition();
    }

    virtual void addChild(IGUIElement* child)
    {
        if (child)
        {
            Children.push_back(child);
            child->grab();
        }
    }

    virtual void removeChild(IGUIElement* child)
    {
        for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
        {
            if (*it == child)
            {
                child->drop();
                Children.erase(it);
                return;
            }
        }
    }

    virtual void removeAllChildren()
    {
        for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
        {
            (*it)->Parent = 0;
            (*it)->remove();
            (*it)->drop();
        }
        Children.clear();
    }

    //! Removes this element from its parent.
    virtual void remove();
    virtual void draw();
    virtual void move(core::CPosition2d<int> offset);
    virtual void moveTo(core::CPosition2d<int> position);
    virtual void centerOnRect(const core::CRect<int>& rect);
    virtual void centerOnParent();
    virtual bool isVisible();
    virtual void setVisible(bool visible);
    virtual bool isEnabled();
    virtual void setEnabled(bool enabled);
    virtual bool isFixed();
    virtual void setFixed(bool fixed);
    virtual bool isInvisible();
    virtual void setInvisible(bool invisible);
    virtual bool doesReportOnDraw();
    virtual void setReportOnDraw(int report);
    virtual void setText(const wchar_t* text);
    virtual const wchar_t* getText() const;
    virtual int getID();
    virtual void setID(int id);
    virtual int getType();
    virtual bool OnEvent(const event::SEvent& event);
    virtual bool OnEventInNonFocusState(const event::SEvent& event);
    virtual bool bringToFront(IGUIElement* element);
    virtual const std::list<IGUIElement*>& getChildren();
    virtual IGUIElement* getElementFromId(int id, bool searchChildren);
    virtual IGUIElement* getHoverItem();
    virtual void setHoverItem(IGUIElement* item);
    virtual core::CDimension2d<int> getPreferredSize();

    core::CRect<int> getAbsolutePosition() { return AbsoluteRect; }
    core::CRect<int> getRelativePosition() { return RelativeRect; }
    core::CRect<int> getAbsoluteClippingRect() { return AbsoluteClippingRect; }

protected:
    std::list<IGUIElement*> Children;
    IGUIElement* Parent;
    core::CRect<int> RelativeRect;
    //! Set when the relative rectangle changes size.
    bool RelativeSizeChanged;
    core::CRect<int> AbsoluteRect;
    core::CRect<int> AbsoluteClippingRect;
    bool IsVisible;
    bool IsEnabled;
    bool IsFixed;
    bool IsInvisible;
    bool NoClip;
    int ReportOnDraw;
    core::CString<wchar_t> Text;
    int ID;
    int Type;
    IGUIEnvironment* Environment;
    IGUIElement* HoverItem;

public:
    //! Layout hints read by the IGUILayout sorters, such as "center br" or "tab".
    const char* LayoutFlags;
    //! Receives the events of a list box's items.
    event::IEventReceiver* EventReceiver;
};

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
    RelativeRect.UpperLeftCorner += offset;
    RelativeRect.LowerRightCorner += offset;
    updateAbsolutePosition();
}

inline void IGUIElement::moveTo(core::CPosition2d<int> position)
{
    setRelativePosition(core::CRect<int>(position.X, position.Y,
        RelativeRect.LowerRightCorner.X + position.X - RelativeRect.UpperLeftCorner.X,
        RelativeRect.LowerRightCorner.Y + position.Y - RelativeRect.UpperLeftCorner.Y));
}

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

//! Offers the event to the event receiver, then to the parent.
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
    IGUIElement* element = 0;
    for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
    {
        if ((*it)->getID() == id)
            return (*it);

        if (searchChildren)
            element = (*it)->getElementFromId(id, true);

        if (element)
            return element;
    }

    return element;
}

inline IGUIElement* IGUIElement::getHoverItem()
{
    return HoverItem;
}

//! Sets the element shown while the mouse is over this one; it starts hidden and unclipped.
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

} // end namespace gui
} // end namespace ox

#endif
