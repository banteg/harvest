// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_GUI_CSTATISTICSSCREEN_H
#define HARVEST_GUI_CSTATISTICSSCREEN_H

#include "ox/event/IEventReceiver.h"

namespace ox { class IOxDevice; }

namespace harvest {
namespace gui {

//! The statistics window shown after a game.
class CStatisticsScreen : public ox::event::IEventReceiver
{
public:
    CStatisticsScreen(ox::IOxDevice* device);
    virtual ~CStatisticsScreen();

    virtual bool OnEvent(const ox::event::SEvent& event);

    void setVisible(bool visible);
    bool isVisible();
    void update(float time);

private:
    // Not recovered yet; keeps the Linux object size of 760 bytes.
    char Unrecovered[760 - sizeof(ox::event::IEventReceiver)];
};

} // end namespace gui
} // end namespace harvest

#endif
