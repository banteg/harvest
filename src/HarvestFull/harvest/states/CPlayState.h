// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only the constructor, the key table and the object size are recovered.

#ifndef HARVEST_STATES_CPLAYSTATE_H
#define HARVEST_STATES_CPLAYSTATE_H

#include "ox/game/CGameState.h"

namespace harvest {
namespace states {

//! The game state of a running game.
class CPlayState : public ox::game::CGameState
{
public:
    CPlayState();
    virtual ~CPlayState();

    virtual bool OnEvent(const ox::event::SEvent& event);
    virtual int firstInit(ox::IOxDevice* device);
    virtual void renderFirst();
    virtual int secondInit();
    virtual int updateState(float time);
    virtual void render();

    //! Which keys are held down, by key code.
    static bool m_keys[256];

private:
    // Not recovered yet; keeps the Linux object size of 2120 bytes.
    char Unrecovered[2120 - sizeof(ox::game::CGameState)];
};

} // end namespace states
} // end namespace harvest

#endif
