// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only the constructor and the object size are recovered.

#ifndef HARVEST_STATES_CMAINMENUSTATE_H
#define HARVEST_STATES_CMAINMENUSTATE_H

#include "ox/game/CGameState.h"

namespace harvest {
namespace states {

//! The main menu with the planet selection state.
class CMainMenuState : public ox::game::CGameState
{
public:
    CMainMenuState();
    virtual ~CMainMenuState();

    virtual bool OnEvent(const ox::event::SEvent& event);
    virtual int firstInit(ox::IOxDevice* device);
    virtual void renderFirst();
    virtual int secondInit();
    virtual int updateState(float time);
    virtual void render();

private:
    // Not recovered yet; keeps the Linux object size of 1000 bytes.
    char Unrecovered[1000 - sizeof(ox::game::CGameState)];
};

} // end namespace states
} // end namespace harvest

#endif
