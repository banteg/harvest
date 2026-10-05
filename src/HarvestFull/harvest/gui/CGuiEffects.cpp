// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "CGuiEffects.h"
#include "ox/entity/COxEntity.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {
namespace gui {

void CGuiEffects::renderRecangleOverlay(ox::video::IVideoDriver* driver,
    const ox::TList<ox::entity::COxEntity*>& entities, const ox::core::CPosition2d<float>& offset,
    int entityType, int radius)
{
    ox::TList<ox::core::CRect<int> > overlay;
    ox::TList<ox::core::CRect<int> > holes;
    ox::core::CRect<int> screen(ox::core::CPosition2d<int>(0, 0), driver->getScreenSize());
    overlay.push_back(screen);

    for (std::list<ox::entity::COxEntity*>::const_iterator it = entities.begin(); it != entities.end(); ++it)
    {
        if ((*it)->getEntityType() == entityType)
        {
            const ox::core::CVector3d<float>& position = (*it)->getPosition();
            ox::core::CRect<int> hole((int)(position.X - offset.X) - radius, (int)(position.Y - offset.Y - position.Z) - radius,
                (int)(position.X - offset.X) + radius, (int)(position.Y - offset.Y - position.Z) + radius);
            if (hole.LowerRightCorner.Y > 0 && hole.UpperLeftCorner.Y < screen.LowerRightCorner.Y &&
                hole.LowerRightCorner.X > 0 && hole.UpperLeftCorner.X < screen.LowerRightCorner.X)
            {
                splitRects(overlay, hole);
                holes.push_back(hole);
            }
        }
    }

    for (std::list<ox::core::CRect<int> >::iterator it = overlay.begin(); it != overlay.end(); ++it)
        driver->draw2DRectangle(ox::video::SColor(0x80000020), *it, 0);
}

void CGuiEffects::splitRects(ox::TList<ox::core::CRect<int> >& rects, const ox::core::CRect<int>& cut)
{
    ox::TList<ox::core::CRect<int> > parts;
    std::list<ox::core::CRect<int> >::iterator it = rects.begin();
    while (it != rects.end())
    {
        if (cut.isRectCollided(*it))
        {
            ox::core::CRect<int> rect = *it;
            it = rects.erase(it);
            if (rect.UpperLeftCorner.X < cut.UpperLeftCorner.X)
            {
                parts.push_back(ox::core::CRect<int>(rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y,
                    cut.UpperLeftCorner.X, rect.LowerRightCorner.Y));
                rect.UpperLeftCorner.X = cut.UpperLeftCorner.X;
            }
            if (rect.LowerRightCorner.X > cut.LowerRightCorner.X)
            {
                parts.push_back(ox::core::CRect<int>(cut.LowerRightCorner.X, rect.UpperLeftCorner.Y,
                    rect.LowerRightCorner.X, rect.LowerRightCorner.Y));
                rect.LowerRightCorner.X = cut.LowerRightCorner.X;
            }
            if (rect.UpperLeftCorner.Y < cut.UpperLeftCorner.Y)
            {
                parts.push_back(ox::core::CRect<int>(rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y,
                    rect.LowerRightCorner.X, cut.UpperLeftCorner.Y));
            }
            if (rect.LowerRightCorner.Y > cut.LowerRightCorner.Y)
            {
                parts.push_back(ox::core::CRect<int>(rect.UpperLeftCorner.X, cut.LowerRightCorner.Y,
                    rect.LowerRightCorner.X, rect.LowerRightCorner.Y));
            }
        }
        else
            ++it;
    }

    for (std::list<ox::core::CRect<int> >::iterator it = parts.begin(); it != parts.end(); ++it)
        rects.push_back(*it);
}

} // end namespace gui
} // end namespace harvest
