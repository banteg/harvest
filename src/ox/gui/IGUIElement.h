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
    virtual void setRelativePosition(const core::CRect<int>& position);
    virtual core::CRect<int> getParentAbsoluteClippingRect(bool clip);
    virtual void updateAbsolutePosition();
    virtual void addChild(IGUIElement* child);
    virtual void removeChild(IGUIElement* child);
    virtual void removeAllChildren();
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
};

} // end namespace gui
} // end namespace ox

#endif
