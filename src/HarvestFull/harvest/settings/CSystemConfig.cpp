// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>
#include <errno.h>
#include <iostream>
#include "harvest/game/CWorld.h"
#include "CSystemConfig.h"
#include "ox/IOxDevice.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/core/CStringFunctions.h"
#include "ox/game/CTextLocalization.h"
#include "ox/io/IFileList.h"
#include "ox/io/IFileSystem.h"
#include "ox/video/IVideoDriver.h"
#include "harvest/entity/CAlienEntity.h"

namespace harvest {
namespace settings {

CSystemConfig* gp_systemConfig;

CSystemConfig::CSystemConfig(ox::io::IFileSystem* fileSystem, const char* filename)
    : FileSystem(fileSystem), Filename(filename), Config(0), Language(0), DefaultLanguage(0)
{
    Config = new ox::game::CConfiguration(fileSystem);
    if (fileSystem->existFile(filename, false) == true)
        Config->read(filename);

    openDefaultLanguage();

    if (Config->attributeExists(L"settings:language") == true)
    {
        ox::core::CString<wchar_t> language;
        Config->getAttribute(L"settings:language", language);
        openLanguageFile(ox::core::CString<char>(language.c_str()));
    }
    else
        openLanguageFile(ox::core::CString<char>(L"$GAME_RESOURCES$/harvestClientData/lang/english.cfg"));

    ox::core::CDimension2d<int> resolution(800, 600);
    if (Config->attributeExists(L"settings:resolutionh") == true &&
        Config->attributeExists(L"settings:resolutionv"))
    {
        resolution = getScreenRes();
        if (resolution.Width <= 0 || resolution.Width % 2 != 0 || resolution.Height <= 0 ||
            resolution.Height % 2 != 0)
            setScreenRes(resolution);
    }
    else
        setScreenRes(resolution);

    if (!Config->attributeExists(L"settings:shaderlevel"))
        setShaderLevel(ESL_HIGH);
    if (!Config->attributeExists(L"settings:fullscreen"))
        Config->setAttribute(L"settings:fullscreen", 0);
    if (!Config->attributeExists(L"settings:driver"))
        Config->setAttribute(L"settings:driver", L"OGL");
    if (Config->attributeExists(L"settings:volumesfx") != true)
        Config->setAttribute(L"settings:volumesfx", 3);
    else if ((unsigned int)Config->getAttributeAsInt(L"settings:volumesfx") > 5)
        Config->setAttribute(L"settings:volumesfx", 3);
    if (Config->attributeExists(L"settings:volumemusic") != true)
        Config->setAttribute(L"settings:volumemusic", 3);
    else if ((unsigned int)Config->getAttributeAsInt(L"settings:volumemusic") > 5)
        Config->setAttribute(L"settings:volumemusic", 3);
    if (Config->attributeExists(L"settings:particles") != true)
        Config->setAttribute(L"settings:particles", 2);
    else if ((unsigned int)Config->getAttributeAsInt(L"settings:particles") > 2)
        Config->setAttribute(L"settings:particles", 2);
    if (!Config->attributeExists(L"settings:scrollspeed"))
        Config->setAttribute(L"settings:scrollspeed", 1.0f);
}

bool CSystemConfig::openDefaultLanguage()
{
    if (DefaultLanguage)
        delete DefaultLanguage;
    DefaultLanguage = performOpenLanguageFile(
        ox::core::CString<char>(L"$GAME_RESOURCES$/harvestClientData/lang/english.cfg"));
    return DefaultLanguage != 0;
}

bool CSystemConfig::openLanguageFile(const ox::core::CString<char>& filename)
{
    if (Language)
        delete Language;
    Language = performOpenLanguageFile(filename);
    if (!Language)
        return false;

    for (int i = 0; i <= 13; ++i)
        entity::setAlienName(i, getLocalizedText(entity::ALIEN_KEY_NAMES[i]).c_str());

    if (Config)
        Config->setAttribute(L"settings:language", ox::core::CString<wchar_t>(filename.c_str()));
    return true;
}

void CSystemConfig::setScreenRes(const ox::core::CDimension2d<int>& resolution)
{
    Config->setAttribute(L"settings:resolutionh", resolution.Width);
    Config->setAttribute(L"settings:resolutionv", resolution.Height);
}

ox::core::CDimension2d<int> CSystemConfig::getScreenRes()
{
    ox::core::CDimension2d<int> resolution;
    resolution.Width = Config->getAttributeAsInt(L"settings:resolutionh");
    resolution.Height = Config->getAttributeAsInt(L"settings:resolutionv");
    return resolution;
}

void CSystemConfig::setShaderLevel(EShaderLevel level)
{
    Config->setAttribute(L"settings:shaderlevel", (int)level);
}

CSystemConfig::~CSystemConfig()
{
    saveConfig();
    if (Config)
        delete Config;
    if (Language)
        delete Language;
    if (DefaultLanguage)
        delete DefaultLanguage;
}

void CSystemConfig::saveConfig()
{
    Config->write(Filename.c_str());
}

bool CSystemConfig::isFirstRun()
{
    bool firstRun = true;
    if (Config->attributeExists(L"settings:firstrun"))
        firstRun = Config->getAttributeAsInt(L"settings:firstrun") != 0;
    return firstRun;
}

void CSystemConfig::setFirstRun(bool firstRun)
{
    Config->setAttribute(L"settings:firstrun", (int)firstRun);
}

ox::core::CString<wchar_t> CSystemConfig::getRecentProfile()
{
    ox::core::CString<wchar_t> profile;
    Config->getAttribute(L"user:recentProfile", profile);
    return ox::core::CString<wchar_t>(profile);
}

ox::core::CString<wchar_t> CSystemConfig::getLicenseKeySetting()
{
    ox::core::CString<wchar_t> key;
    Config->getAttributeFromBase64(L"settings:system", key);
    return ox::core::CString<wchar_t>(key);
}

void CSystemConfig::addLicenseKeySetting(const ox::core::CString<wchar_t>& key)
{
    Config->setAttributeAsBase64(ox::core::CString<wchar_t>(L"settings:system"), key);
}

ox::game::CTextLocalization* CSystemConfig::getCurrentLanguage()
{
    return Language;
}

bool CSystemConfig::getFullscreen()
{
    return Config->getAttributeAsInt(L"settings:fullscreen") != 0;
}

ox::core::CString<wchar_t> CSystemConfig::getVideoDriver()
{
    ox::core::CString<wchar_t> driver;
    Config->getAttribute(L"settings:driver", driver);
    return driver;
}

int CSystemConfig::getSfxVolume()
{
    return Config->getAttributeAsInt(L"settings:volumesfx");
}

int CSystemConfig::getMusicVolume()
{
    return Config->getAttributeAsInt(L"settings:volumemusic");
}

int CSystemConfig::getShaderLevel()
{
    return Config->getAttributeAsInt(L"settings:shaderlevel");
}

int CSystemConfig::getParticleSetting()
{
    return Config->getAttributeAsInt(L"settings:particles");
}

float CSystemConfig::getScrollSpeed()
{
    return Config->getAttributeAsFloat(L"settings:scrollspeed");
}

void CSystemConfig::setRecentProfile(const ox::core::CString<wchar_t>& profile)
{
    Config->setAttribute(ox::core::CString<wchar_t>(L"user:recentProfile"), profile);
}

void CSystemConfig::setFullscreen(bool fullscreen)
{
    Config->setAttribute(L"settings:fullscreen", (int)fullscreen);
}

void CSystemConfig::setVideoDriver(const wchar_t* driver)
{
    Config->setAttribute(L"settings:driver", driver);
}

void CSystemConfig::setSfxVolume(int volume)
{
    Config->setAttribute(L"settings:volumesfx", volume);
}

void CSystemConfig::setMusicVolume(int volume)
{
    Config->setAttribute(L"settings:volumemusic", volume);
}

void CSystemConfig::setParticleSetting(int setting)
{
    Config->setAttribute(L"settings:particles", setting);
}

void CSystemConfig::setScrollSpeed(float speed)
{
    Config->setAttribute(L"settings:scrollspeed", speed);
}

void CSystemConfig::applySettings(ox::IOxDevice* device)
{
    device->getAudioDriver()->setMusicVolume(getMusicVolume());
    device->getAudioDriver()->setSoundEffectVolume(getSfxVolume());
    ox::core::CDimension2d<int> resolution = getScreenRes();
    ox::core::CDimension2d<int> screenSize = device->getVideoDriver()->getScreenSize();
    getFullscreen();
    device->getVideoDriver()->isFullscreen();
    if (resolution != screenSize)
        device->resizeDeviceWindow(resolution);
}

ox::core::CString<wchar_t> CSystemConfig::getCurrentLanguageName()
{
    ox::core::CString<wchar_t> name = L"";
    if (Language)
        Language->getAttribute(L"global:languageFilename", name);
    return name;
}

ox::core::CString<wchar_t> CSystemConfig::getCurrentLanguageDisplayName()
{
    ox::core::CString<wchar_t> name = L"";
    if (Language)
        Language->getAttribute(L"global:languageName", name);
    return name;
}

ox::core::CString<wchar_t> CSystemConfig::getCurrentLanguageTag()
{
    ox::core::CString<wchar_t> tag = L"";
    if (Language)
        Language->getAttribute(L"global:languageTag", tag);
    return tag;
}

ox::game::CTextLocalization* CSystemConfig::performOpenLanguageFile(const ox::core::CString<char>& filename)
{
    ox::game::CTextLocalization* language = new ox::game::CTextLocalization(FileSystem);
    if (!language->read(filename.c_str()))
    {
        if (language)
            delete language;
        return 0;
    }
    return language;
}

ox::core::CString<wchar_t> CSystemConfig::getLocalizedText(const wchar_t* key, ...)
{
    ox::core::CString<wchar_t> text;
    ox::game::CTextLocalization* language = Language ? Language : DefaultLanguage;
    if (language)
    {
        text = language->getText(key);
        if (text.size() == 0)
        {
            text = DefaultLanguage->getText(key);
            if (text.size() == 0)
            {
                text.append(ox::core::CString<wchar_t>(L"??? "));
                text.append(ox::core::CString<wchar_t>(key));
                text.append(ox::core::CString<wchar_t>(L" ???"));
                return ox::core::CString<wchar_t>(text);
            }
        }

        text = ox::core::replaceAll(text, ox::core::CString<wchar_t>(L"%s"), ox::core::CString<wchar_t>(L"%S"));
        va_list args;
        va_start(args, key);
        wchar_t buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        // The Mac build reads errno here; the Linux errno accessor is const, so the read is dropped.
        if (vswprintf(buffer, 1024, text.c_str(), args) < 0)
            errno;
        va_end(args);
        return ox::core::CString<wchar_t>(buffer);
    }

    ox::core::CString<wchar_t> missing = L"no lang ??? ";
    missing.append(ox::core::CString<wchar_t>(key));
    missing.append(ox::core::CString<wchar_t>(L" ???"));
    return ox::core::CString<wchar_t>(missing);
}

void CSystemConfig::getAllLanguages(ox::TArray<SLanguageFile>& languages)
{
    ox::core::CString<char> directory = "$GAME_RESOURCES$/harvestClientData/lang/";
    ox::io::IFileList* files = FileSystem->createFileList("*.cfg", directory.c_str(), (ox::io::EFileList)1);
    for (int i = 0; i < files->getFileCount(); ++i)
    {
        ox::game::CConfiguration* config = new ox::game::CConfiguration(FileSystem);
        ox::core::CString<char> filename = "$GAME_RESOURCES$/harvestClientData/lang/";
        filename.append(ox::core::CString<char>(files->getFileName(i)));
        printf(filename.c_str());
        if (config->read(filename.c_str()) == true)
        {
            if (config->attributeExists(L"global:languageName") == true)
            {
                ox::core::CString<wchar_t> name;
                config->getAttribute(L"global:languageName", name);
                SLanguageFile language;
                language.Name = name;
                language.Filename = filename;
                languages.push_back(language);
            }
        }
        if (config)
            delete config;
    }
    files->drop();
}

} // end namespace settings
} // end namespace harvest
