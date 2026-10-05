// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_SETTINGS_CHARVESTPROFILE_H
#define HARVEST_SETTINGS_CHARVESTPROFILE_H

#include "ox/core/CString.h"

namespace harvest {
namespace settings {

//! A player profile.
class CHarvestProfile
{
public:
    ox::core::CString<wchar_t> getPlayerName();
    ox::core::CString<wchar_t> getPlayerGroup();
};

} // end namespace settings
} // end namespace harvest

#endif
