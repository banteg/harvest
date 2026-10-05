// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CHarvestEntity.h"
#include "CHarvestFullMain.h"
#include "harvest/game/CStatistics.h"
#include "harvest/settings/CProfileManager.h"
#include "harvest/settings/CSystemConfig.h"
#include "harvest/states/CIntroState.h"
#include "harvest/states/CMainMenuState.h"
#include "harvest/states/CPlayState.h"
#include "harvest/states/CShuttleRaceState.h"
#include "harvest/game/CThreatLevel.h"
#include "ox/IOSOperator.h"
#include "ox/IOxDevice.h"
#include "ox/ITimer.h"
#include "ox/algo/CRand.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUISkin.h"
#include "ox/io/IFileSystem.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/gui/IGUIElement.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {

int g_gameMode;
int g_gamePlanet;
ox::core::CString<char> g_loadGameFilename;
int g_scenarioResultGameMode;
int g_scenarioResultPlanet;
int g_scenarioResult = -1;

CHarvestFullMain::CHarvestFullMain()
    : SuperReceiver(0)
{
}

CHarvestFullMain::~CHarvestFullMain()
{
    if (SuperReceiver)
        delete SuperReceiver;
    if (settings::gp_profileManager)
        delete settings::gp_profileManager;
    if (settings::gp_systemConfig)
        delete settings::gp_systemConfig;
    if (game::gp_statistics)
        delete game::gp_statistics;
    game::gp_statistics = 0;
    game::CHighscoreInfo::setNewHighscoreInfo(0);
    if (Device)
    {
        Device->drop();
        Device = 0;
    }
}

int CHarvestFullMain::init()
{
    Device = ox::createDevice(ox::video::EDT_OPENGL, 0, L"0.7");
    if (!Device)
        return 1;

    ox::core::CString<char> userDataPath = Device->getOSOperator()->getApplicationSupportPath("Harvest");
    ox::io::IFileSystem* fileSystem = Device->getFileSystem();
    fileSystem->addDirectoryAlias("$HARVEST_USERDATA$", userDataPath.c_str());
    if (!fileSystem->existFile("$HARVEST_USERDATA$", false))
        fileSystem->createDirectory("$HARVEST_USERDATA$");
    if (!fileSystem->existFile("$HARVEST_USERDATA$/profiles/", false))
        fileSystem->createDirectory("$HARVEST_USERDATA$/profiles/");
    if (!fileSystem->existFile("$HARVEST_USERDATA$/screenshots/", false))
        fileSystem->createDirectory("$HARVEST_USERDATA$/screenshots/");
    if (!fileSystem->existFile("$HARVEST_USERDATA$/sandbox", false))
        fileSystem->createDirectory("$HARVEST_USERDATA$/sandbox");

    settings::gp_systemConfig = new settings::CSystemConfig(Device->getFileSystem(), "$HARVEST_USERDATA$/harvest.cfg");
    if (settings::gp_systemConfig->isFirstRun())
    {
        ox::TArray<settings::SLanguageFile> languages;
        settings::gp_systemConfig->getAllLanguages(languages);
        ox::TArray<ox::core::CString<wchar_t> > languageNames;
        for (unsigned int i = 0; i < languages.size(); ++i)
            languageNames.push_back(languages[i].Name);

        if (!Device->createUserSelectedDeviceWindow(&languageNames, 101))
            return 1;
        settings::gp_systemConfig->setFirstRun(false);
        settings::gp_systemConfig->setFullscreen(Device->getVideoDriver()->isFullscreen());
        settings::gp_systemConfig->setScreenRes(Device->getVideoDriver()->getScreenSize());
        if (Device->getVideoDriver()->getDriverType() == ox::video::EDT_DIRECTX9)
            settings::gp_systemConfig->setVideoDriver(L"DX9");
        else if (Device->getVideoDriver()->getDriverType() == ox::video::EDT_OPENGL)
            settings::gp_systemConfig->setVideoDriver(L"OGL");
        else
            return 1;

        int language = Device->getSelectedLanguageIndex();
        if (language >= 0 && language < (int)languages.size())
            settings::gp_systemConfig->openLanguageFile(languages[language].Filename);
        settings::gp_systemConfig->saveConfig();
    }
    else
    {
        ox::core::CDimension2d<int> resolution = settings::gp_systemConfig->getScreenRes();
        bool fullscreen = settings::gp_systemConfig->getFullscreen();
        ox::core::CString<wchar_t> driver = settings::gp_systemConfig->getVideoDriver();
        if (driver == ox::core::CString<wchar_t>(L"DX9"))
            Device->setVideoDriver(ox::video::EDT_DIRECTX9);
        else if (driver == ox::core::CString<wchar_t>(L"OGL"))
            Device->setVideoDriver(ox::video::EDT_OPENGL);
        else
            return 1;
        if (!Device->createDeviceWindow(resolution, 32, fullscreen, false, false, 101))
            return 1;
    }

    settings::gp_profileManager = new settings::CProfileManager(Device);
    Device->getVideoDriver()->queryFeature((ox::video::E_VIDEO_DRIVER_FEATURE)0);
    ox::algo::CRand::srand(Device->getTimer()->getTime());
    SuperReceiver = new CHarvestSuperReceiver(Device);

    if (Device->getAudioDriver())
    {
        ox::core::CString<char> soundPath = Device->getFileSystem()->getDirectoryFromAlias("$GAME_RESOURCES$");
        soundPath.append(ox::core::CString<char>("/harvestClientData/sfx/"));
        Device->getAudioDriver()->setSoundEffectPath(soundPath.c_str());
    }

    settings::gp_systemConfig->applySettings(Device);
    ox::gui::IGUISkin* skin = Device->getGUIEnvironment()->getSkin();
    skin->setDefaultText(ox::gui::EGDT_MSG_BOX_YES, settings::gp_systemConfig->getLocalizedText(L"menu:yes").c_str());
    skin->setDefaultText(ox::gui::EGDT_MSG_BOX_OK, settings::gp_systemConfig->getLocalizedText(L"menu:ok").c_str());
    skin->setDefaultText(ox::gui::EGDT_MSG_BOX_NO, settings::gp_systemConfig->getLocalizedText(L"menu:no").c_str());
    skin->setDefaultText(ox::gui::EGDT_MSG_BOX_CANCEL,
        settings::gp_systemConfig->getLocalizedText(L"menu:cancel").c_str());

    Running = true;
    g_gameMode = game::EGM_CREATIVE;
    g_gamePlanet = 0;
    setState(EGS_INTRO);
    return 0;
}

void CHarvestFullMain::updateMain(float time)
{
}

ox::game::CGameState* CHarvestFullMain::stateFactory(int state)
{
    switch (state)
    {
    case EGS_INTRO:
        return new states::CIntroState();
    case EGS_MAIN_MENU:
        return new states::CMainMenuState();
    case EGS_PLAY:
        return new states::CPlayState();
    case EGS_SHUTTLE_RACE:
        return new states::CShuttleRaceState();
    }
    return 0;
}

CHarvestSuperReceiver::CHarvestSuperReceiver(ox::IOxDevice* device)
    : Device(device)
{
    subscribe(ox::event::gp_subscriberList);
}

bool CHarvestSuperReceiver::OnEvent(const ox::event::SEvent& event)
{
    bool result = false;
    if (event.EventType == ox::event::EET_GUI_EVENT)
    {
        if (event.GUIEvent.EventType == ox::gui::EGET_CHECKBOX_CHANGED)
        {
            if (event.GUIEvent.Caller->getType() == 18)
                Device->getAudioDriver();
        }
        else if (event.GUIEvent.EventType == ox::gui::EGET_BUTTON_CLICKED ||
            event.GUIEvent.EventType == ox::gui::EGET_TEXT_BUTTON_CLICKED)
        {
            if (Device->getAudioDriver())
                Device->getAudioDriver()->playSound("BtnPressed.ogg", 1.0f, 0.0f, 1.0f);
        }
    }
    else if (event.EventType == ox::event::EET_KEY_INPUT_EVENT)
    {
        if (event.KeyInput.Event == ox::event::EKIE_KEY_LEFT_UP && event.KeyInput.Key == ox::KEY_KEY_T &&
            event.KeyInput.Control)
        {
            ox::core::CString<char> directory = "$HARVEST_USERDATA$/screenshots/";
            Device->getVideoDriver()->saveJpegScreenshot(directory.c_str(), 0);
            result = true;
        }
        else if (event.KeyInput.Event == ox::event::EKIE_TOGGLE_FULLSCREEN)
        {
            ox::event::SEvent toggled;
            toggled.EventType = ox::event::EET_DEVICE_EVENT;
            toggled.DeviceEvent.Type = ox::event::EDE_FULLSCREEN_TOGGLED;
            ox::event::gp_subscriberList->postDelayedEvent(toggled);
            Device->setFullscreenMode(!Device->getVideoDriver()->isFullscreen());
        }
    }
    return result;
}

} // end namespace harvest
