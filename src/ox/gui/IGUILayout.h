// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: the virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIWindow up to updateChildrenForContentArea.

#ifndef OX_GUI_IGUILAYOUT_H
#define OX_GUI_IGUILAYOUT_H

#include "IGUIElement.h"

namespace ox {
namespace gui {

//! An element that arranges its children by their layout hints.
class IGUILayout : public IGUIElement
{
public:
    virtual void sortFlow();
    virtual void sortVertically();
    virtual void sortHorizontally();
    //! Places the children in rows broken at "br" hints, aligned by their other hints.
    virtual void sortRiver(bool resize, int spacingX, int spacingY, bool sortHidden);
    virtual core::CRect<int> getContentArea();
    virtual void updateChildrenForContentArea();
};

} // end namespace gui
} // end namespace ox

#endif
