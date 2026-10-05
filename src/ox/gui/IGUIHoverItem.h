// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only placeHoverItem is known (it is inlined into daisy::gui::CGUIEnvironment::drawAll).

#ifndef OX_GUI_IGUIHOVERITEM_H
#define OX_GUI_IGUIHOVERITEM_H

#include "IGUIElement.h"
#include "IGUIElementInline.h"

namespace ox {
namespace gui {

//! A hover description shown above the element it describes.
class IGUIHoverItem
{
public:
    //! Centers item above element and keeps it inside its parent: below element when there is no
    //! room above, and shifted sideways at the left and right edges.
    static void placeHoverItem(IGUIElement* element, IGUIElement* item)
    {
        if (element && item)
        {
            core::CRect<int> area = element->getAbsolutePosition();
            core::CRect<int> itemRect = item->getRelativePosition();
            item->moveTo(core::CPosition2d<int>(area.getWidth() / 2 + area.UpperLeftCorner.X - itemRect.getWidth() / 2,
                area.UpperLeftCorner.Y - 10 - itemRect.getHeight()));

            IGUIElement* parent = item->getParent();
            if (parent)
            {
                core::CRect<int> bounds = parent->getAbsolutePosition();
                core::CRect<int> placed = item->getAbsolutePosition();

                if (placed.UpperLeftCorner.Y < bounds.UpperLeftCorner.Y)
                    item->move(core::CPosition2d<int>(0, area.getHeight() + placed.getHeight() + 20));

                placed = item->getAbsolutePosition();
                if (placed.UpperLeftCorner.X < bounds.UpperLeftCorner.X)
                    item->move(core::CPosition2d<int>(bounds.UpperLeftCorner.X - placed.UpperLeftCorner.X, 0));
                else if (placed.LowerRightCorner.X > bounds.LowerRightCorner.X)
                    item->move(core::CPosition2d<int>(bounds.LowerRightCorner.X - placed.LowerRightCorner.X, 0));
            }
        }
    }
};

} // end namespace gui
} // end namespace ox

#endif
