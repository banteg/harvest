// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_GUI_CGUIEFFECTS_H
#define HARVEST_GUI_CGUIEFFECTS_H

#include "ox/TList.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CRect.h"

namespace ox {
namespace entity { class COxEntity; }
namespace video { class IVideoDriver; }
} // end namespace ox

namespace harvest {
namespace gui {

//! Screen effects shared by the gui screens.
class CGuiEffects
{
public:
    //! Darkens the screen except for a square of the given radius around every entity of the type.
    static void renderRecangleOverlay(ox::video::IVideoDriver* driver,
        const ox::TList<ox::entity::COxEntity*>& entities, const ox::core::CPosition2d<float>& offset,
        int entityType, int radius);
    //! Replaces every rectangle that overlaps the cut by the parts of it outside the cut.
    static void splitRects(ox::TList<ox::core::CRect<int> >& rects, const ox::core::CRect<int>& cut);
};

} // end namespace gui
} // end namespace harvest

#endif
