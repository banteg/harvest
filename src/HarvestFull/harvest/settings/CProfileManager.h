// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_SETTINGS_CPROFILEMANAGER_H
#define HARVEST_SETTINGS_CPROFILEMANAGER_H

namespace ox { class IOxDevice; }

namespace harvest {
namespace settings {

//! The player profiles.
class CProfileManager
{
public:
    CProfileManager(ox::IOxDevice* device);
    virtual ~CProfileManager();

private:
    // Not recovered yet; keeps the Linux object size of 56 bytes.
    char Unrecovered[56 - sizeof(void*)];
};

extern CProfileManager* gp_profileManager;

} // end namespace settings
} // end namespace harvest

#endif
