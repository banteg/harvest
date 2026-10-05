// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The Mac 1.18 vtable of ox::gui::IGUIClickArea adds no slots to IGUILayout's.

#ifndef OX_GUI_IGUICLICKAREA_H
#define OX_GUI_IGUICLICKAREA_H

#include "IGUILayout.h"

namespace ox {
namespace gui {

//! An invisible element that reports the mouse buttons pressed and released over it to its parent.
class IGUIClickArea : public IGUILayout
{
public:
    IGUIClickArea(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUILayout(environment, parent, id, rectangle)
    {
        Type = EGUIET_CLICK_AREA;
    }
};

} // end namespace gui
} // end namespace ox

#endif
