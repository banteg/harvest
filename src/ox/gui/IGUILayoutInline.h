// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Inline constructors of the layout interfaces, which the element implementations inline; kept apart
// from the interface headers like IGUIElementInline.h.

#ifndef OX_GUI_IGUILAYOUTINLINE_H
#define OX_GUI_IGUILAYOUTINLINE_H

#include "IGUIElementInline.h"
#include "IGUILayout.h"
#include "IGUIWindow.h"

namespace ox {
namespace gui {

inline IGUILayout::IGUILayout(IGUIEnvironment* environment, IGUIElement* parent, int id,
    core::CRect<int> rectangle)
    : IGUIElement(environment, parent, id, rectangle)
{
    Type = EGUIET_LAYOUT;
}

inline IGUIWindow::IGUIWindow(IGUIEnvironment* environment, IGUIElement* parent, int id,
    core::CRect<int> rectangle)
    : IGUILayout(environment, parent, id, rectangle)
{
    Type = EGUIET_WINDOW;
}

} // end namespace gui
} // end namespace ox

#endif
