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

    virtual core::CRect<int> getParentAbsoluteClippingRect(bool clip);
    virtual void sortFlow(int spacingX, int spacingY, bool resize, bool sortHidden);
    //! Stacks the children that are not fixed, centered when center is set, and grows to fit them.
    virtual void sortVertically(int spacing, bool center)
    {
        core::CRect<int> area = getContentArea();
        int width = area.getWidth();
        int x = 0;
        int y = spacing;

        for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
        {
            if ((*it)->isFixed())
                continue;

            core::CRect<int> rect = (*it)->getRelativePosition();
            if (center)
                x = (width - rect.getWidth()) / 2;

            core::CRect<int> sorted(core::CPosition2d<int>(x, y), core::CDimension2d<int>(rect.getWidth(), rect.getHeight()));
            (*it)->setRelativePosition(sorted);
            y += sorted.getHeight() + spacing;
        }

        int overflow = y + area.UpperLeftCorner.Y - area.LowerRightCorner.Y;
        if (overflow > 0)
            RelativeRect.LowerRightCorner.Y += overflow;
        setRelativePosition(RelativeRect);
        updateChildrenForContentArea();
    }

    virtual void sortHorizontally(int spacing);
    //! Places the children in rows broken at "br" hints, aligned by their other hints.
    virtual void sortRiver(bool resize, int spacingX, int spacingY, bool sortHidden);
    //! The rectangle the children are sorted in, relative to the element.
    virtual core::CRect<int> getContentArea();
    virtual void updateChildrenForContentArea();
};

} // end namespace gui
} // end namespace ox

#endif
