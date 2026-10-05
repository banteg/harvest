// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIElement.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. Partial: the virtual order
// follows the Mac 1.18 vtable of ox::gui::IGUIElement; members are not recovered yet.

#ifndef OX_GUI_IGUIELEMENT_H
#define OX_GUI_IGUIELEMENT_H

#include "../IUnknown.h"
#include "../core/CPosition2d.h"
#include "../core/CRect.h"
#include "../event/IEventReceiver.h"

namespace ox {
namespace gui {

//! Base class of all GUI elements.
class IGUIElement : public IUnknown
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
    virtual void setReportOnDraw(bool report);
    virtual void setText(const wchar_t* text);
    virtual const wchar_t* getText() const;
    virtual int getID();
    virtual void setID(int id);
    virtual int getType();
    virtual bool OnEvent(const event::SEvent& event);
    virtual bool OnEventInNonFocusState(const event::SEvent& event);
    virtual bool bringToFront(IGUIElement* element);
};

} // end namespace gui
} // end namespace ox

#endif
