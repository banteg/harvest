// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what CPlayState uses is declared; the tail keeps the Linux object size.

#ifndef HARVEST_GUI_CSAVEGAMESCREEN_H
#define HARVEST_GUI_CSAVEGAMESCREEN_H

#include "ox/event/IEventReceiver.h"

namespace ox {
class IOxDevice;
} // end namespace ox

namespace harvest {
namespace gui {

//! The save and load game screen.
class CSaveGameScreen : public ox::event::IEventReceiver
{
public:
    CSaveGameScreen(ox::IOxDevice* device);
    virtual ~CSaveGameScreen();

    virtual bool OnEvent(const ox::event::SEvent& event);
    bool isVisible();
    void setVisible(bool visible, bool save);

private:
    // Not recovered yet; keeps the Linux object size of 232 bytes.
    char Unrecovered[232 - sizeof(ox::event::IEventReceiver)];
};

} // end namespace gui
} // end namespace harvest

#endif
