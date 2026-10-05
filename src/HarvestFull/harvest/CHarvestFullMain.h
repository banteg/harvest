// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The Linux build has no Steam callbacks; member names are inferred.

#ifndef HARVEST_CHARVESTFULLMAIN_H
#define HARVEST_CHARVESTFULLMAIN_H

#include "ox/core/CString.h"
#include "ox/event/IEventReceiver.h"
#include "ox/game/CGameMain.h"

namespace harvest {

//! The game mode of the running game, as in harvest::game::EGAME_MODE.
extern int g_gameMode;
//! The planet of the game to start.
extern int g_gamePlanet;
//! The save game to load when the game starts, empty for a new game.
extern ox::core::CString<char> g_loadGameFilename;
//! The game mode and planet of the last finished scenario.
extern int g_scenarioResultGameMode;
extern int g_scenarioResultPlanet;
//! The outcome of the last scenario, -1 when it was left early.
extern int g_scenarioResult;

//! Handles the input every state shares: button sounds, screenshots and the fullscreen toggle.
class CHarvestSuperReceiver : public ox::event::IEventReceiver
{
public:
    CHarvestSuperReceiver(ox::IOxDevice* device);

    virtual bool OnEvent(const ox::event::SEvent& event);

private:
    ox::IOxDevice* Device;
};

//! The game: creates the device and settings and switches between the game states.
class CHarvestFullMain : public ox::game::CGameMain
{
public:
    CHarvestFullMain();
    virtual ~CHarvestFullMain();

    virtual void updateMain(float time);
    virtual int init();
    virtual ox::game::CGameState* stateFactory(int state);

private:
    CHarvestSuperReceiver* SuperReceiver;
};

} // end namespace harvest

#endif
