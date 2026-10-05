// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what CPlayState uses is declared; the tail keeps the Linux object size.

#ifndef HARVEST_GUI_CSETTINGSSCREEN_H
#define HARVEST_GUI_CSETTINGSSCREEN_H

#include "ox/event/IEventReceiver.h"

namespace ox {
class IOxDevice;
} // end namespace ox

namespace harvest {
namespace gui {

//! The settings screen.
class CSettingsScreen : public ox::event::IEventReceiver
{
public:
    CSettingsScreen(ox::IOxDevice* device, bool mainMenu);
    virtual ~CSettingsScreen();

    virtual bool OnEvent(const ox::event::SEvent& event);
    void setVisible(bool visible);

private:
    // Not recovered yet; keeps the Linux object size of 64 bytes.
    char Unrecovered[64 - sizeof(ox::event::IEventReceiver)];
};

} // end namespace gui
} // end namespace harvest

#endif
