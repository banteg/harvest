// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_SETTINGS_CHARVESTPROFILE_H
#define HARVEST_SETTINGS_CHARVESTPROFILE_H

#include "ox/Keycodes.h"
#include "ox/core/CString.h"

namespace harvest {
namespace settings {

//! A player profile with its key bindings and weapon priorities.
class CHarvestProfile
{
public:
    //! Returns the game command bound to the key.
    int getCommandForKey(ox::EKEY_CODE key);
    //! Stores a priority set in the CAlienPriorities string form.
    void setAttackPriority(int index, const ox::core::CString<wchar_t>& priorities);
    void setAttackRangeMatters(int index, bool matters);
};

} // end namespace settings
} // end namespace harvest

#endif
