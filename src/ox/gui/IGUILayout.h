// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: the virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIWindow up to sortRiver.

#ifndef OX_GUI_IGUILAYOUT_H
#define OX_GUI_IGUILAYOUT_H

#include "IGUIElement.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

//! An element that arranges its children by their layout hints.
class IGUILayout : public IGUIElement
{
public:
    IGUILayout(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
    }

    //! Without clipping, the children are clipped by the content area instead of the clipping rectangle.
    virtual core::CRect<int> getParentAbsoluteClippingRect(bool clip);
    //! Places the children in rows, from the right when rightToLeft; resize widens the element to fit.
    virtual void sortFlow(int spacingX, int spacingY, bool rightToLeft, bool resize);
    //! Stacks the children, horizontally centered when center is set.
    virtual void sortVertically(int spacing, bool center);
    //! Places the children side by side, vertically centered.
    virtual void sortHorizontally(int spacing);
    //! Places the children in rows broken at "br" hints, aligned by their other hints; "tab" hints line up
    //! in columns. Children without hints count as "tab" when tabUnflagged is set.
    virtual void sortRiver(bool resize, int spacingX, int spacingY, bool tabUnflagged);
    //! The area of the children, relative to the element.
    virtual core::CRect<int> getContentArea();
    //! Moves the movable children by the content area's offset.
    virtual void updateChildrenForContentArea();
};

} // end namespace gui
} // end namespace ox

#endif
