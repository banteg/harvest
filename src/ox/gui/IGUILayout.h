// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIWindow up to
// updateChildrenForContentArea. The methods are inline; their copies are emitted in
// daisy/gui/CGUIEnvironment.cpp. The sorters are defined in IGUILayoutInline.h.

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
    inline IGUILayout(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle);

    virtual core::CRect<int> getParentAbsoluteClippingRect(bool clip)
    {
        if (IsInvisible && Parent)
            return Parent->getParentAbsoluteClippingRect(clip);

        if (clip)
            return AbsoluteClippingRect;

        // the content area in absolute coordinates, clipped against the parent
        core::CRect<int> rect = getContentArea();
        rect.UpperLeftCorner.X += AbsoluteRect.UpperLeftCorner.X;
        rect.UpperLeftCorner.Y += AbsoluteRect.UpperLeftCorner.Y;
        rect.LowerRightCorner.X += AbsoluteRect.UpperLeftCorner.X;
        rect.LowerRightCorner.Y += AbsoluteRect.UpperLeftCorner.Y;

        if (Parent)
        {
            core::CRect<int> parentClip = Parent->getParentAbsoluteClippingRect(IsFixed);
            if (parentClip.LowerRightCorner.X < rect.LowerRightCorner.X)
                rect.LowerRightCorner.X = parentClip.LowerRightCorner.X;
            if (parentClip.LowerRightCorner.Y < rect.LowerRightCorner.Y)
                rect.LowerRightCorner.Y = parentClip.LowerRightCorner.Y;
            if (parentClip.UpperLeftCorner.X > rect.UpperLeftCorner.X)
                rect.UpperLeftCorner.X = parentClip.UpperLeftCorner.X;
            if (parentClip.UpperLeftCorner.Y > rect.UpperLeftCorner.Y)
                rect.UpperLeftCorner.Y = parentClip.UpperLeftCorner.Y;
        }

        return rect;
    }

    //! Places the children in rows from the left, or from the right, wrapping at the content width,
    //! and grows the element to fit them (in width too when resize is set).
    virtual inline void sortFlow(int spacingX, int spacingY, bool rightToLeft, bool resize);

    //! Stacks the children from the top, centered horizontally when center is set, and grows the
    //! element to fit them.
    virtual inline void sortVertically(int spacing, bool center);

    //! Lines the children up from the left, centered vertically, and grows the element to fit them.
    virtual inline void sortHorizontally(int spacing);

    //! Places the children at their preferred sizes in rows broken at "br" hints, lines up the
    //! elements hinted "tab" across the rows, aligns each row by its last "left", "center",
    //! "right", "top", "middle" or "bottom" hint and grows the element to fit when resize is set.
    //! Children without hints are only aligned when sortHidden is set.
    virtual inline void sortRiver(bool resize, int spacingX, int spacingY, bool sortHidden);

    //! The area children are placed in, relative to the element.
    virtual core::CRect<int> getContentArea()
    {
        return core::CRect<int>(0, 0, RelativeRect.getWidth(), RelativeRect.getHeight());
    }

    //! Moves the movable children by the content area's offset.
    virtual void updateChildrenForContentArea()
    {
        core::CPosition2d<int> offset = getContentArea().UpperLeftCorner;
        for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
            if (!(*it)->isFixed())
                (*it)->move(offset);
    }
};

} // end namespace gui
} // end namespace ox

#endif
