// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_SETTINGS_CHARVESTPROFILE_H
#define HARVEST_SETTINGS_CHARVESTPROFILE_H

#include "ox/Keycodes.h"
#include "ox/core/CString.h"

namespace harvest {
namespace settings {

//! The game commands that can be bound to keys.
enum EKeyCommands
{
    EKC_NONE = 0,
    EKC_INCREASE_SPEED,
    EKC_DECREASE_SPEED,
    EKC_SPEED_PAUSED,
    EKC_SPEED_SLOWER,
    EKC_SPEED_NORMAL,
    EKC_SPEED_FASTER,
    EKC_SPEED_FASTEST,
    EKC_SPEED_PAUSE_TOGGLE,
    EKC_BUILD_PRODUCER,
    EKC_BUILD_MOVER,
    EKC_BUILD_MINER,
    EKC_BUILD_TOWER,
    EKC_BUILD_LAUNCHER,
    EKC_ACTION_SPECIAL,
    EKC_ACTION_EAGLE,
    EKC_ACTION_TEMPEST,
    EKC_ACTION_SELL,
    EKC_ACTION_SOMETHING,
    EKC_GAME_SETTINGS,
    EKC_GAME_PRIORITIES,
    EKC_GAME_RANGES,
    EKC_GAME_OVERHEATS,
    EKC_COUNT
};

//! Localization keys of the key commands, indexed by EKeyCommands.
static const wchar_t* const KeyCommandNames[EKC_COUNT] =
{
    L"-",
    L"commandString:increaseSpeed",
    L"commandString:decreaseSpeed",
    L"commandString:speedPaused",
    L"commandString:speedSlower",
    L"commandString:speedNormal",
    L"commandString:speedFaster",
    L"commandString:speedFastest",
    L"commandString:speedPauseToggle",
    L"commandString:buildProducer",
    L"commandString:buildMover",
    L"commandString:buildMiner",
    L"commandString:buildTower",
    L"commandString:buildLauncher",
    L"commandString:actionSpecial",
    L"commandString:actionEagle",
    L"commandString:actionTempest",
    L"commandString:actionSell",
    L"commandString:actionSomething",
    L"commandString:gameSettings",
    L"commandString:gamePriorities",
    L"commandString:gameRanges",
    L"commandString:gameOverheats"
};

//! A player profile: settings, key bindings and progress.
class CHarvestProfile
{
public:
    virtual ~CHarvestProfile();

    //! Binds the key to the command. Returns false if the key cannot be bound. Mac has the key type
    //! as ox::input::EKEY_CODE.
    bool makeKeyMapping(ox::EKEY_CODE key, EKeyCommands command);
    //! The key bound to the command, or 0.
    int getKeyForCommand(EKeyCommands command);
    //! The highscore group the player belongs to, or an empty string.
    ox::core::CString<wchar_t> getPlayerGroup();
};

} // end namespace settings
} // end namespace harvest

#endif
