// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_GUI_CHIGHSCORESCREEN_H
#define HARVEST_GUI_CHIGHSCORESCREEN_H

#include "ox/event/IEventReceiver.h"

namespace ox { class IOxDevice; }

namespace harvest {
namespace gui {

//! The online highscore window of the main menu.
class CHighscoreScreen : public ox::event::IEventReceiver
{
public:
    CHighscoreScreen(ox::IOxDevice* device);
    virtual ~CHighscoreScreen();

    void update(float time);

    virtual bool OnEvent(const ox::event::SEvent& event);

    void setVisible(bool visible);
    bool isVisible();

private:
    // Not recovered yet; keeps the Linux object size of 6760 bytes.
    char Unrecovered[6760 - sizeof(ox::event::IEventReceiver)];
};

} // end namespace gui
} // end namespace harvest

#endif
