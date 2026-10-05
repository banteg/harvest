// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: init and stateFactory are not recovered yet.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CHarvestEntity.h"
#include "CHarvestFullMain.h"
#include "harvest/game/CStatistics.h"
#include "harvest/settings/CProfileManager.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/IOxDevice.h"
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

void CHarvestFullMain::updateMain(float time)
{
}

CHarvestSuperReceiver::CHarvestSuperReceiver(ox::IOxDevice* device)
    : Device(device)
{
    subscribe(ox::event::gp_subscriberList);
}

bool CHarvestSuperReceiver::OnEvent(const ox::event::SEvent& event)
{
    if (event.EventType == ox::event::EET_GUI_EVENT)
    {
        if (event.GUIEvent.EventType == ox::gui::EGET_BUTTON_CLICKED ||
            event.GUIEvent.EventType == ox::gui::EGET_TEXT_BUTTON_CLICKED)
        {
            if (Device->getAudioDriver())
                Device->getAudioDriver()->playSound("BtnPressed.ogg", 1.0f, 0.0f, 1.0f);
        }
        else if (event.GUIEvent.EventType == ox::gui::EGET_CHECKBOX_CHANGED)
        {
            if (event.GUIEvent.Caller->getType() == 18)
                Device->getAudioDriver();
        }
    }
    else if (event.EventType == ox::event::EET_KEY_INPUT_EVENT)
    {
        if (event.KeyInput.Event == ox::event::EKIE_KEY_LEFT_UP && event.KeyInput.Key == ox::KEY_KEY_T &&
            event.KeyInput.Control)
        {
            ox::core::CString<char> directory = "$HARVEST_USERDATA$/screenshots/";
            Device->getVideoDriver()->saveJpegScreenshot(directory.c_str(), 0);
            return true;
        }
        if (event.KeyInput.Event == ox::event::EKIE_TOGGLE_FULLSCREEN)
        {
            ox::event::SEvent toggled;
            toggled.EventType = ox::event::EET_DEVICE_EVENT;
            toggled.DeviceEvent.Type = ox::event::EDE_FULLSCREEN_TOGGLED;
            ox::event::gp_subscriberList->postDelayedEvent(toggled);
            Device->setFullscreenMode(!Device->getVideoDriver()->isFullscreen());
        }
    }
    return false;
}

} // end namespace harvest
