// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "CProfileManager.h"
#include "CHarvestProfile.h"
#include "CSystemConfig.h"
#include "ox/IOxDevice.h"
#include "ox/core/CBasic.h"
#include "ox/game/CConfiguration.h"
#include "ox/io/IFileList.h"
#include "ox/io/IFileSystem.h"

namespace harvest {
namespace settings {

CProfileManager* gp_profileManager;

CProfileManager::~CProfileManager()
{
    writeCurrentProfile();
    delete CurrentProfile;
    for (unsigned int i = 0; i < Profiles.size(); ++i)
    {
        if (Profiles[i])
            delete Profiles[i];
    }
}

ox::TArray<SProfileName*>& CProfileManager::getProfileNames(bool refresh)
{
    if (refresh || Profiles.empty())
        createProfileList();
    return Profiles;
}

void CProfileManager::writeCurrentProfile()
{
    if (CurrentProfile)
        CurrentProfile->writeProfile();
}

CProfileManager::CProfileManager(ox::IOxDevice* device)
    : Device(device), CurrentProfile(0)
{
    FileSystem = device->getFileSystem();
    ox::core::CString<wchar_t> recent = gp_systemConfig->getRecentProfile();
    if (recent.size() != 0)
        openProfile(ox::core::CString<char>(recent.c_str()));
}

CHarvestProfile* CProfileManager::getCurrentProfile()
{
    return CurrentProfile;
}

bool CProfileManager::deleteCurrentProfile()
{
    if (!CurrentProfile)
        return false;
    FileSystem->deleteFile(CurrentProfile->getFilename().c_str());
    if (CurrentProfile)
        delete CurrentProfile;
    CurrentProfile = 0;
    return true;
}

bool CProfileManager::createProfile(const ox::core::CString<wchar_t>& name)
{
    if (CurrentProfile)
        delete CurrentProfile;
    CurrentProfile = new CHarvestProfile();

    ox::core::CString<char> filename;
    int number = 0;
    do
    {
        filename = "$HARVEST_USERDATA$/profiles/";
        filename.append(ox::core::CString<char>("Profile-"));
        filename.append(ox::core::CBasic::getTimeString("%y%m%d"));
        if (number < 10)
            filename.append(ox::core::CString<char>("-0"));
        else
            filename.append(ox::core::CString<char>("-"));
        filename.append(number);
        filename.append(ox::core::CString<char>(".cfg"));
        ++number;
    } while (FileSystem->existFile(filename.c_str(), false));

    CurrentProfile->createNewProfile(name, FileSystem, filename);
    SProfileName* profile = new SProfileName;
    profile->Name = name;
    profile->Filename = filename;
    Profiles.push_back(profile);
    if (gp_systemConfig)
        gp_systemConfig->setRecentProfile(ox::core::CString<wchar_t>(filename.c_str()));
    return true;
}

void CProfileManager::createProfileList()
{
    for (unsigned int i = 0; i < Profiles.size(); ++i)
    {
        if (Profiles[i])
            delete Profiles[i];
    }
    Profiles.clear();

    ox::core::CString<char> directory = "$HARVEST_USERDATA$/profiles/";
    ox::io::IFileList* files = FileSystem->createFileList("*.cfg", directory.c_str(), (ox::io::EFileList)1);
    CHarvestProfile profile;
    for (int i = 0; i < files->getFileCount(); ++i)
    {
        ox::game::CConfiguration* config = new ox::game::CConfiguration(FileSystem);
        if (profile.openProfile(FileSystem, ox::core::CString<char>(files->getFullFileName(i))) == true)
        {
            SProfileName* name = new SProfileName;
            name->Filename = directory;
            name->Filename.append(ox::core::CString<char>(files->getFileName(i)));
            name->Name = profile.getPlayerName();
            Profiles.push_back(name);
        }
        delete config;
    }
    files->drop();
}

bool CProfileManager::openProfile(const ox::core::CString<char>& filename)
{
    if (CurrentProfile)
    {
        CurrentProfile->writeProfile();
        if (CurrentProfile)
            delete CurrentProfile;
    }
    CurrentProfile = new CHarvestProfile();
    if (!CurrentProfile->openProfile(FileSystem, filename))
    {
        if (CurrentProfile)
            delete CurrentProfile;
        CurrentProfile = 0;
        return false;
    }
    if (gp_systemConfig)
        gp_systemConfig->setRecentProfile(ox::core::CString<wchar_t>(filename.c_str()));
    return true;
}

bool CProfileManager::openProfileByName(const ox::core::CString<wchar_t>& name)
{
    for (ox::TArray<SProfileName*>::iterator it = Profiles.begin(); it != Profiles.end(); ++it)
    {
        if ((*it)->Name == name)
            return openProfile((*it)->Filename);
    }
    return false;
}

} // end namespace settings
} // end namespace harvest
