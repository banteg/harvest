// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_GUI_CPROFILESCREEN_H
#define HARVEST_GUI_CPROFILESCREEN_H

#include "ox/event/IEventReceiver.h"

namespace ox { class IOxDevice; }

namespace harvest {
namespace gui {

//! The windows that create, select and delete player profiles.
class CProfileScreen : public ox::event::IEventReceiver
{
public:
    CProfileScreen(ox::IOxDevice* device);
    virtual ~CProfileScreen();

    virtual bool OnEvent(const ox::event::SEvent& event);

    void createEditProfileWindow(bool firstProfile);
    void createProfileListWindow();
    bool isVisible();

private:
    // Not recovered yet; keeps the Linux object size of 56 bytes.
    char Unrecovered[56 - sizeof(ox::event::IEventReceiver)];
};

} // end namespace gui
} // end namespace harvest

#endif
