// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_CHARVESTFULLMAIN_H
#define HARVEST_CHARVESTFULLMAIN_H

namespace harvest {

//! The game mode of the running game, as in harvest::game::EGAME_MODE.
extern int g_gameMode;
//! The planet of the game to start.
extern int g_gamePlanet;
//! The outcome of the last scenario, -1 when it was left early.
extern int g_scenarioResult;

} // end namespace harvest

#endif
