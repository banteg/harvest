// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: refillMainView is not recovered yet.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "CMenuInfoDialog.h"
#include "harvest/CHarvestFullMain.h"
#include "ox/IOxDevice.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/gui/IGUILayout.h"
#include "ox/gui/IGUISkin.h"

namespace harvest {
namespace gui {

CMenuInfoDialog::CMenuInfoDialog(ox::IOxDevice* device)
    : Device(device), Window(0), Mode(0), SyndicateTime(0), InfoTime(0)
{
    GUIEnvironment = device->getGUIEnvironment();
    Driver = device->getVideoDriver();
}

CMenuInfoDialog::~CMenuInfoDialog()
{
    if (Window)
        Window->remove();
    if (Device)
    {
        if (Device->getAudioDriver() && Music.size() > 0)
            Device->getAudioDriver()->stopMusic(Music.c_str());
        if (Device && Device->getAudioDriver())
            Device->getAudioDriver()->stopVoice();
    }
}

bool CMenuInfoDialog::OnEvent(const ox::event::SEvent& event)
{
    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
    {
        int id = event.GUIEvent.Caller->getID();
        if (event.GUIEvent.EventType == ox::gui::EGET_BUTTON_CLICKED)
        {
            if (id == ID_START_TUTORIAL)
            {
                setVisible(false, EIM_WELCOME);
                g_gamePlanet = 0;
                g_gameMode = 5;
                ox::event::SEvent startEvent;
                startEvent.EventType = ox::event::EET_USER_EVENT;
                startEvent.UserEvent.UserData1 = 12;
                startEvent.UserEvent.UserData2 = 0;
                startEvent.UserEvent.UserData3 = 0;
                startEvent.UserEvent.UserPointer = 0;
                ox::event::gp_subscriberList->OnEvent(startEvent);
                return true;
            }
            if (id == ID_CLOSE)
            {
                setVisible(false, EIM_WELCOME);
                ox::event::SEvent closeEvent;
                closeEvent.EventType = ox::event::EET_USER_EVENT;
                closeEvent.UserEvent.UserData1 = 4;
                closeEvent.UserEvent.UserData2 = 0;
                closeEvent.UserEvent.UserData3 = 0;
                closeEvent.UserEvent.UserPointer = 0;
                ox::event::gp_subscriberList->OnEvent(closeEvent);
                g_scenarioResult = -1;
                return true;
            }
        }
        else if (event.GUIEvent.EventType == ox::gui::EGET_ELEMENT_DRAWN)
        {
            if ((id == ID_SYNDICATE_TEXT && SyndicateTime > 0) || (id == ID_INFO_TEXT && InfoTime > 0))
            {
                ox::core::CRect<int> rect = event.GUIEvent.Caller->getAbsolutePosition();
                int dots = (int)((id == ID_SYNDICATE_TEXT ? SyndicateTime : InfoTime) * 1000) / 500 % 4;
                switch (dots)
                {
                case 1:
                    GUIEnvironment->getSkin()->getFont()->draw(L".", rect, ox::video::SColor(0xffffffff),
                        ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER, 0);
                    break;
                case 2:
                    GUIEnvironment->getSkin()->getFont()->draw(L"...", rect, ox::video::SColor(0xffffffff),
                        ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER, 0);
                    break;
                case 3:
                    GUIEnvironment->getSkin()->getFont()->draw(L".....", rect, ox::video::SColor(0xffffffff),
                        ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER, 0);
                    break;
                }
            }
        }
        break;
    }
    case ox::event::EET_MOUSE_INPUT_EVENT:
    case ox::event::EET_KEY_INPUT_EVENT:
        return true;
    default:
        break;
    }
    return false;
}

void CMenuInfoDialog::setVisible(bool visible, int mode)
{
    if (visible)
    {
        if (!Window)
        {
            Window = GUIEnvironment->addModalScreen();
            Frame = GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 400, 400), Window, -1);
        }
        refillMainView(mode);
        if (Device->getAudioDriver())
        {
            if (mode == EIM_WELCOME)
                Device->getAudioDriver()->playVoice("medusa1_phone_line.ogg");
            else if (mode == EIM_DEBRIEFING)
                Device->getAudioDriver()->playVoice("medusa2_phone_line.ogg");
        }
        Window->setVisible(true);
    }
    else if (Window)
    {
        Window->setVisible(false);
        if (Device && Device->getAudioDriver())
            Device->getAudioDriver()->stopVoice();
    }
}

void CMenuInfoDialog::update(float time)
{
    SyndicateTime -= time;
    if (InfoTime > 0)
    {
        InfoTime -= time;
        if (InfoTime < 0 && Device->getAudioDriver())
        {
            if (Mode == EIM_WELCOME)
                Device->getAudioDriver()->playVoice("scenario_infoWelcome.ogg");
            else if (Mode == EIM_DEBRIEFING)
                Device->getAudioDriver()->playVoice("scenario_infoDebriefing.ogg");
        }
    }
}

bool CMenuInfoDialog::isVisible()
{
    if (Window)
        return Window->isVisible();
    return false;
}

} // end namespace gui
} // end namespace harvest
