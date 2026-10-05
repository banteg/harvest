// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_SETTINGS_CPROFILEMANAGER_H
#define HARVEST_SETTINGS_CPROFILEMANAGER_H

namespace harvest {
namespace settings {

//! The player profiles.
class CProfileManager
{
public:
    virtual ~CProfileManager();
};

extern CProfileManager* gp_profileManager;

} // end namespace settings
} // end namespace harvest

#endif
