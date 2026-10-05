// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Method names are from the Mac symbols; member names are ours.

#ifndef HARVEST_SETTINGS_CSYSTEMCONFIG_H
#define HARVEST_SETTINGS_CSYSTEMCONFIG_H

#include "ox/TArray.h"
#include "ox/core/CString.h"
#include "ox/core/CDimension2d.h"

namespace ox {
class IOxDevice;
namespace io { class IFileSystem; }
namespace game { class CConfiguration; class CTextLocalization; }
} // end namespace ox

namespace harvest {
namespace settings {

enum EShaderLevel
{
    ESL_NONE = 0,
    ESL_LOW = 1,
    ESL_HIGH = 2
};

//! A language file found by getAllLanguages.
struct SLanguageFile
{
    ox::core::CString<wchar_t> Name;
    ox::core::CString<char> Filename;
};

//! The game's settings file and its localized texts.
class CSystemConfig
{
public:
    CSystemConfig(ox::io::IFileSystem* fileSystem, const char* filename);
    bool openDefaultLanguage();
    bool openLanguageFile(const ox::core::CString<char>& filename);
    void setScreenRes(const ox::core::CDimension2d<int>& resolution);
    ox::core::CDimension2d<int> getScreenRes();
    void setShaderLevel(EShaderLevel level);
    virtual ~CSystemConfig();
    void saveConfig();
    bool isFirstRun();
    void setFirstRun(bool firstRun);
    ox::core::CString<wchar_t> getRecentProfile();
    void setRecentProfile(const ox::core::CString<wchar_t>& profile);
    ox::core::CString<wchar_t> getLicenseKeySetting();
    void addLicenseKeySetting(const ox::core::CString<wchar_t>& key);
    ox::game::CTextLocalization* getCurrentLanguage();
    bool getFullscreen();
    ox::core::CString<wchar_t> getVideoDriver();
    int getSfxVolume();
    int getMusicVolume();
    int getShaderLevel();
    int getParticleSetting();
    float getScrollSpeed();
    void setFullscreen(bool fullscreen);
    void setVideoDriver(const wchar_t* driver);
    void setSfxVolume(int volume);
    void setMusicVolume(int volume);
    void setParticleSetting(int setting);
    void setScrollSpeed(float speed);
    void applySettings(ox::IOxDevice* device);
    ox::core::CString<wchar_t> getCurrentLanguageName();
    ox::core::CString<wchar_t> getCurrentLanguageDisplayName();
    ox::core::CString<wchar_t> getCurrentLanguageTag();
    ox::game::CTextLocalization* performOpenLanguageFile(const ox::core::CString<char>& filename);
    //! The text for a localization key, with the arguments filled in.
    ox::core::CString<wchar_t> getLocalizedText(const wchar_t* key, ...);
    void getAllLanguages(ox::TArray<SLanguageFile>& languages);

private:
    ox::io::IFileSystem* FileSystem;
    ox::core::CString<char> Filename;
    ox::game::CConfiguration* Config;
    ox::game::CTextLocalization* Language;
    //! English, for keys the selected language lacks.
    ox::game::CTextLocalization* DefaultLanguage;
};

extern CSystemConfig* gp_systemConfig;

} // end namespace settings
} // end namespace harvest

#endif
