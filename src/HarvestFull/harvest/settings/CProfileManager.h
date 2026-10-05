// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Method names are from the Mac symbols; member names are ours.

#ifndef HARVEST_SETTINGS_CPROFILEMANAGER_H
#define HARVEST_SETTINGS_CPROFILEMANAGER_H

#include "ox/TArray.h"
#include "ox/core/CString.h"
// Not needed by the declarations; CHarvestFullMain.cpp only matches with it included.
#include "ox/io/IFileList.h"

namespace ox {
class IOxDevice;
namespace io { class IFileSystem; }
} // end namespace ox

namespace harvest {
namespace settings {

class CHarvestProfile;

//! A profile found in the profiles directory.
struct SProfileName
{
    ox::core::CString<wchar_t> Name;
    ox::core::CString<char> Filename;
};

//! The player profiles and the open one.
class CProfileManager
{
public:
    CProfileManager(ox::IOxDevice* device);
    bool openProfile(const ox::core::CString<char>& filename);
    virtual ~CProfileManager();
    void writeCurrentProfile();
    //! The profiles, listed again when refresh is set or none are known.
    ox::TArray<SProfileName*>& getProfileNames(bool refresh);
    void createProfileList();
    CHarvestProfile* getCurrentProfile();
    bool openProfileByName(const ox::core::CString<wchar_t>& name);
    bool createProfile(const ox::core::CString<wchar_t>& name);
    bool deleteCurrentProfile();

private:
    ox::IOxDevice* Device;
    ox::io::IFileSystem* FileSystem;
    ox::TArray<SProfileName*> Profiles;
    CHarvestProfile* CurrentProfile;
};

extern CProfileManager* gp_profileManager;

} // end namespace settings
} // end namespace harvest

#endif
