// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_SETTINGS_CHARVESTPROFILE_H
#define HARVEST_SETTINGS_CHARVESTPROFILE_H

#include "ox/core/CString.h"

namespace harvest {
namespace settings {

//! A player profile with its settings, scores and achievements.
class CHarvestProfile
{
public:
    //! The best score of a kind (0 levels, 1 minerals, 2 time in milliseconds) for a game mode and
    //! planet, or 0 when there is none.
    int getLocalScore(int kind, int gameMode, int planet);
    void updateLocalScore(int kind, int gameMode, int planet, int score);
    ox::core::CString<wchar_t> getPlayerName();
    ox::core::CString<wchar_t> getPlayerGroup();
    //! The attack priorities of a tower group, as read by CAlienPriorities::setPrioritiesFromString.
    ox::core::CString<wchar_t> getAttackPriority(int group);
    bool getAttackRangeMatters(int group);
};

} // end namespace settings
} // end namespace harvest

#endif
