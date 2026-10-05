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
    EGUIET_LIST_BOX = 8
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

    //! Draws the visible children; ReportOnDraw 1 reports before and 2 after drawing them.
    virtual void draw()
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
    //! Receives the events of this element before its parent does.
    event::IEventReceiver* EventReceiver;
};

} // end namespace gui
} // end namespace ox

#endif
