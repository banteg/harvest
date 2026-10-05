// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only the constructor and the object size are recovered.

#ifndef HARVEST_STATES_CSHUTTLERACESTATE_H
#define HARVEST_STATES_CSHUTTLERACESTATE_H

#include "ox/game/CGameState.h"

namespace harvest {
namespace states {

//! The shuttle race minigame state.
class CShuttleRaceState : public ox::game::CGameState
{
public:
    CShuttleRaceState();
    virtual ~CShuttleRaceState();

    virtual bool OnEvent(const ox::event::SEvent& event);
    virtual int firstInit(ox::IOxDevice* device);
    virtual void renderFirst();
    virtual int secondInit();
    virtual int updateState(float time);
    virtual void render();

private:
    // Not recovered yet; keeps the Linux object size of 632 bytes.
    char Unrecovered[632 - sizeof(ox::game::CGameState)];
};

} // end namespace states
} // end namespace harvest

#endif
