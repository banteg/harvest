// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what CPlayState uses is declared; the tail keeps the Linux object size.

#ifndef HARVEST_GUI_CPRIORITYSCREEN_H
#define HARVEST_GUI_CPRIORITYSCREEN_H

#include "ox/event/IEventReceiver.h"

namespace ox {
class IOxDevice;
} // end namespace ox

namespace harvest {
namespace game { class CThreatLevel; }
namespace gui {

//! The alien attack priority screen.
class CPriorityScreen : public ox::event::IEventReceiver
{
public:
    CPriorityScreen(ox::IOxDevice* device);
    virtual ~CPriorityScreen();

    virtual bool OnEvent(const ox::event::SEvent& event);
    bool isVisible();
    void setVisible(bool visible, game::CThreatLevel* threatLevel);

private:
    // Not recovered yet; keeps the Linux object size of 648 bytes.
    char Unrecovered[648 - sizeof(ox::event::IEventReceiver)];
};

} // end namespace gui
} // end namespace harvest

#endif
