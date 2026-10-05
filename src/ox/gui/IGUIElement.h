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
    EGUIET_EDIT_BOX = 12,
    EGUIET_TAB = 19
};

//! Base class of all GUI elements.
class IGUIElement : public IUnknown, public event::IEventReceiver
{
public:
    IGUIElement(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
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

    //! Draws the children; elements outside their clipping rectangle are skipped when fixed.
    virtual void draw()
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

    virtual void move(core::CPosition2d<int> offset)
    {
        RelativeRect.UpperLeftCorner.X += offset.X;
        RelativeRect.UpperLeftCorner.Y += offset.Y;
        RelativeRect.LowerRightCorner.X += offset.X;
        RelativeRect.LowerRightCorner.Y += offset.Y;
        updateAbsolutePosition();
    }

    virtual void moveTo(core::CPosition2d<int> position)
    {
        setRelativePosition(core::CRect<int>(position.X, position.Y, position.X + RelativeRect.getWidth(),
            position.Y + RelativeRect.getHeight()));
    }

    //! Moves the element to the center of a rectangle of the parent's size.
    virtual void centerOnRect(const core::CRect<int>& rect)
    {
        moveTo(core::CPosition2d<int>((rect.getWidth() - RelativeRect.getWidth()) / 2,
            (rect.getHeight() - RelativeRect.getHeight()) / 2));
    }

    virtual void centerOnParent()
    {
        centerOnRect(Parent->getRelativePosition());
    }

    virtual bool isVisible()
    {
        return IsVisible;
    }

    virtual void setVisible(bool visible)
    {
        IsVisible = visible;
    }

    virtual bool isEnabled()
    {
        return IsEnabled;
    }

    virtual void setEnabled(bool enabled)
    {
        IsEnabled = enabled;
        for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
            (*it)->setEnabled(enabled);
    }

    virtual bool isFixed()
    {
        return IsFixed;
    }

    virtual void setFixed(bool fixed)
    {
        IsFixed = fixed;
    }

    virtual bool isInvisible()
    {
        return IsInvisible;
    }

    virtual void setInvisible(bool invisible)
    {
        IsInvisible = invisible;
    }

    virtual bool doesReportOnDraw()
    {
        return ReportOnDraw != 0;
    }

    virtual void setReportOnDraw(int report)
    {
        ReportOnDraw = report;
    }

    virtual void setText(const wchar_t* text)
    {
        Text = text;
    }

    virtual const wchar_t* getText() const
    {
        return Text.c_str();
    }

    virtual int getID()
    {
        return ID;
    }

    virtual void setID(int id)
    {
        ID = id;
    }

    virtual int getType()
    {
        return Type;
    }

    //! Offers the event to the event parent, then passes it to the parent.
    virtual bool OnEvent(const event::SEvent& event)
    {
        if (EventParent && EventParent->OnEvent(event))
            return true;

        if (Parent)
            return Parent->OnEvent(event);

        return true;
    }

    virtual bool OnEventInNonFocusState(const event::SEvent& event)
    {
        for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
            if ((*it)->OnEventInNonFocusState(event))
                return true;

        return false;
    }

    virtual bool bringToFront(IGUIElement* element)
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

    virtual const std::list<IGUIElement*>& getChildren()
    {
        return Children;
    }

    virtual IGUIElement* getElementFromId(int id, bool searchChildren)
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

    virtual IGUIElement* getHoverItem()
    {
        return HoverItem;
    }

    //! Sets the element shown while the mouse hovers over this one.
    virtual void setHoverItem(IGUIElement* item)
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

    virtual core::CDimension2d<int> getPreferredSize()
    {
        return core::CDimension2d<int>(RelativeRect.getWidth(), RelativeRect.getHeight());
    }

    IGUIElement* getParent() { return Parent; }
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

protected:
    //! Gets the events before the parent does.
    event::IEventReceiver* EventParent;
};

} // end namespace gui
} // end namespace ox

#endif
