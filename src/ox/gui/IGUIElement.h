// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIElement.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. The virtual order follows the
// Mac 1.18 vtable of ox::gui::IGUIElement and the member offsets its accessors; member names and the
// flag types are inferred. The methods after remove() are inline too: their copies are emitted in
// daisy/gui/CGUIEnvironment.cpp, the first unit whose vtables need them.

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
    EGUIET_BUTTON = 18,
    //! IGUILayout and the elements derived from it.
    EGUIET_LAYOUT = 19
};

//! Base class of all GUI elements.
class IGUIElement : public IUnknown, public event::IEventReceiver
{
public:
    inline IGUIElement(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle);

    // The methods up to remove() are inline, as in Irrlicht, so that remove() is the key function
    // and the vtable and destructors are emitted in IGUIElement.cpp, as in the Linux build. The
    // inline methods after it are defined in IGUIElementInline.h.
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
    virtual inline void draw();
    virtual inline void move(core::CPosition2d<int> offset);
    virtual inline void moveTo(core::CPosition2d<int> position);
    //! Centers the element in a rectangle of rect's size, relative to the parent.
    virtual inline void centerOnRect(const core::CRect<int>& rect);
    virtual inline void centerOnParent();
    virtual inline bool isVisible();
    virtual inline void setVisible(bool visible);
    virtual inline bool isEnabled();
    virtual inline void setEnabled(bool enabled);
    virtual inline bool isFixed();
    virtual inline void setFixed(bool fixed);
    virtual inline bool isInvisible();
    virtual inline void setInvisible(bool invisible);
    virtual inline bool doesReportOnDraw();
    virtual inline void setReportOnDraw(int report);
    virtual inline void setText(const wchar_t* text);
    virtual inline const wchar_t* getText() const;
    virtual inline int getID();
    virtual inline void setID(int id);
    virtual inline int getType();
    //! Offers the event to the event receiver, then passes it up to the parent.
    virtual inline bool OnEvent(const event::SEvent& event);
    //! Offers the event to the children until one processes it.
    virtual inline bool OnEventInNonFocusState(const event::SEvent& event);
    //! Moves a child to the end of the list, where it is drawn last.
    virtual inline bool bringToFront(IGUIElement* element);
    virtual inline const std::list<IGUIElement*>& getChildren();
    virtual inline IGUIElement* getElementFromId(int id, bool searchChildren);
    virtual inline IGUIElement* getHoverItem();
    //! Sets the element shown while this one is hovered; it starts hidden and unclipped.
    virtual inline void setHoverItem(IGUIElement* item);
    virtual inline core::CDimension2d<int> getPreferredSize();
    //! Returns the topmost visible element at the point, searching the children back to front.
    inline IGUIElement* getElementFromPoint(const core::CPosition2d<int>& point);

    IGUIElement* getParent()
    {
        return Parent;
    }

    core::CRect<int> getAbsolutePosition() { return AbsoluteRect; }
    core::CRect<int> getRelativePosition() { return RelativeRect; }
    core::CRect<int> getAbsoluteClippingRect() { return AbsoluteClippingRect; }

protected:
    //! Clears its children's parent links when destroyed.
    friend class IGUIHoverParent;

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
    //! Offered every event before the parent: the root element for IGUIHoverParent, the owner of a
    //! list box.
    event::IEventReceiver* EventReceiver;
};

} // end namespace gui
} // end namespace ox

#endif
