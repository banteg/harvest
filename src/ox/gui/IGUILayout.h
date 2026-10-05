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
    // Defined inline in IGUILayoutInline.h, which only the element implementations include.
    IGUILayout(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle);

    virtual core::CRect<int> getParentAbsoluteClippingRect(bool clip);
    virtual void sortFlow(int spacingX, int spacingY, bool resize, bool sortHidden);
    virtual void sortVertically(int spacing, bool resize);
    virtual void sortHorizontally(int spacing);
    //! Places the children in rows broken at "br" hints, aligned by their other hints.
    virtual void sortRiver(bool resize, int spacingX, int spacingY, bool sortHidden);
    //! The rectangle inside the frame that children are placed in.
    virtual core::CRect<int> getContentArea();
    virtual void updateChildrenForContentArea();
};

} // end namespace gui
} // end namespace ox

#endif
