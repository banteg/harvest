// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_STATES_CPLAYSTATE_H
#define HARVEST_STATES_CPLAYSTATE_H

namespace harvest {
namespace states {

//! The game state of a running game.
class CPlayState
{
public:
    //! Which keys are held down, by key code.
    static bool m_keys[256];
};

} // end namespace states
} // end namespace harvest

#endif
