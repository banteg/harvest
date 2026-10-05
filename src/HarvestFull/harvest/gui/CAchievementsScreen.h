// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_GUI_CACHIEVEMENTSSCREEN_H
#define HARVEST_GUI_CACHIEVEMENTSSCREEN_H

#include "ox/event/IEventReceiver.h"

namespace ox { class IOxDevice; }

namespace harvest {
namespace gui {

//! The awards window of the main menu.
class CAchievementsScreen : public ox::event::IEventReceiver
{
public:
    CAchievementsScreen(ox::IOxDevice* device);
    virtual ~CAchievementsScreen();

    void update(float time);

    virtual bool OnEvent(const ox::event::SEvent& event);

    void setVisible(bool visible);
    bool isVisible();

private:
    // Not recovered yet; keeps the Linux object size of 928 bytes.
    char Unrecovered[928 - sizeof(ox::event::IEventReceiver)];
};

} // end namespace gui
} // end namespace harvest

#endif
