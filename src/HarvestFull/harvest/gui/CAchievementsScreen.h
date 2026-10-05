// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what CPlayState uses is declared; the tail keeps the Linux object size.

#ifndef HARVEST_GUI_CACHIEVEMENTSSCREEN_H
#define HARVEST_GUI_CACHIEVEMENTSSCREEN_H

#include "ox/event/IEventReceiver.h"

namespace ox {
class IOxDevice;
} // end namespace ox

namespace harvest {
namespace gui {

//! The awards screen.
class CAchievementsScreen : public ox::event::IEventReceiver
{
public:
    CAchievementsScreen(ox::IOxDevice* device);
    virtual ~CAchievementsScreen();

    virtual bool OnEvent(const ox::event::SEvent& event);
    bool isVisible();
    void setVisible(bool visible);
    void update(float frameDelta);

private:
    // Not recovered yet; keeps the Linux object size of 928 bytes.
    char Unrecovered[928 - sizeof(ox::event::IEventReceiver)];
};

} // end namespace gui
} // end namespace harvest

#endif
