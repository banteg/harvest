// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "CMenuInfoDialog.h"
#include "harvest/CHarvestFullMain.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/IOxDevice.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/gui/IGUIButton.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/gui/IGUILayout.h"
#include "ox/gui/IGUISkin.h"
#include "ox/gui/IGUIStaticText.h"
#include "ox/video/IVideoDriver.h"

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

void CMenuInfoDialog::refillMainView(int mode)
{
    if (!Frame)
        return;
    Mode = mode;
    Frame->removeAllChildren();
    if (mode == EIM_WELCOME || mode == EIM_DEBRIEFING)
    {
        ox::core::CString<wchar_t> syndicateName(L"#2");
        syndicateName.append(settings::gp_systemConfig->getLocalizedText(L"characters:syndicate"));
        syndicateName.append(ox::core::CString<wchar_t>(L"#o\n"));
        ox::core::CString<wchar_t> infoName(L"#2");
        infoName.append(settings::gp_systemConfig->getLocalizedText(L"characters:info"));
        infoName.append(ox::core::CString<wchar_t>(L"#o\n"));

        ox::core::CString<wchar_t> syndicateText;
        ox::core::CString<wchar_t> infoText;
        ox::core::CString<wchar_t> question;
        ox::core::CString<wchar_t> yes;
        ox::core::CString<wchar_t> no;
        bool askToStart = false;
        // How long the info speaker's line is revealed, in milliseconds.
        unsigned int infoDuration = 25500;
        if (mode == EIM_WELCOME)
        {
            syndicateText = settings::gp_systemConfig->getLocalizedText(L"introduction:syndicateWelcome");
            infoText = settings::gp_systemConfig->getLocalizedText(L"introduction:infoWelcome");
            question = settings::gp_systemConfig->getLocalizedText(L"introduction:welcomeQuestion");
            yes = settings::gp_systemConfig->getLocalizedText(L"introduction:welcomeYes");
            no = settings::gp_systemConfig->getLocalizedText(L"introduction:welcomeNo");
            askToStart = true;
            infoDuration = 25500;
        }
        else if (mode == EIM_DEBRIEFING)
        {
            syndicateText = settings::gp_systemConfig->getLocalizedText(L"introduction:syndicateSuccess");
            infoText = settings::gp_systemConfig->getLocalizedText(L"introduction:infoSuccess");
            question = settings::gp_systemConfig->getLocalizedText(L"introduction:startGame");
            no = settings::gp_systemConfig->getLocalizedText(L"introduction:startGameBtn");
            askToStart = false;
            infoDuration = 18000;
        }

        SyndicateTime = 4.5f;
        ox::core::CString<wchar_t> text(syndicateName);
        text.append(syndicateText);
        ox::gui::IGUIStaticText* syndicate = GUIEnvironment->addStaticText(text.c_str(),
            ox::core::CRect<int>(0, 0, 400, 400), false, true, Frame, ID_SYNDICATE_TEXT, L"");
        syndicate->setParagraphIcon("PortraitSyndicate",
            Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", false), true);
        syndicate->LayoutFlags = "center";
        syndicate->activateProgressiveReveal(4500);
        syndicate->setReportOnDraw(1);
        InfoTime = infoDuration * 0.001f + 0.75f;

        text = infoName;
        text.append(ox::core::CString<wchar_t>(infoText.c_str()));
        ox::gui::IGUIStaticText* info = GUIEnvironment->addStaticText(text.c_str(),
            ox::core::CRect<int>(0, 0, 400, 400), false, true, Frame, ID_INFO_TEXT, L"");
        info->setParagraphIcon("PortraitCommunications|right",
            Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", false), true);
        info->LayoutFlags = "br";
        info->activateProgressiveReveal(infoDuration);
        info->setReportOnDraw(1);
        GUIEnvironment->addStaticText(question.c_str(), "br", Frame, 0, -1);

        if (askToStart)
        {
            ox::gui::IGUIButton* button = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), Frame,
                ID_START_TUTORIAL, yes.c_str());
            button->LayoutFlags = "center br";
            button->setOverrideFont(GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt"));
        }
        ox::gui::IGUIButton* button = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), Frame,
            ID_CLOSE, no.c_str());
        if (!askToStart)
            button->LayoutFlags = "center br";
        button->setOverrideFont(GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt"));
    }
    else if (mode == EIM_HIGHSCORES)
    {
        ox::gui::IGUIStaticText* text = GUIEnvironment->addStaticText(
            L"The high-score system in Harvest: Massive Encounter will support online and offline lists with "
            L"competitions for both individuals and groups. There will be top-lists for best threat levels, best "
            L"times or simply most minerals for different game modes and planets...\n\nIn the final version of the "
            L"game, that is.",
            ox::core::CRect<int>(0, 0, 400, 400), false, true, Frame, -1, L"");
        text->LayoutFlags = "center";
        text->packSize();
        ox::gui::IGUIStaticText* screen = GUIEnvironment->addStaticText(L" ",
            ox::core::CRect<int>(0, 0, 200, 400), false, true, Frame, -1, L"");
        screen->setParagraphIcon("ScreenHighscores1",
            Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true), true);
        screen->LayoutFlags = "br";
        screen = GUIEnvironment->addStaticText(L" ", ox::core::CRect<int>(0, 0, 200, 400), false, true, Frame, -1,
            L"");
        screen->setParagraphIcon("ScreenHighscores2",
            Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true), true);
        GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), Frame, ID_CLOSE, L"Ok")->LayoutFlags =
            "center br";
    }
    else if (mode == EIM_STATISTICS)
    {
        ox::gui::IGUIStaticText* text = GUIEnvironment->addStaticText(
            L"In the final version of Harvest: Massive Encounter you will be greeted with a statistics screen with "
            L"reports on your performance after each game round. The statistics will contain information about "
            L"buildings, aliens, minerals and threat levels. You will also be notified about your score and "
            L"top-list status.",
            ox::core::CRect<int>(0, 0, 400, 400), false, true, Frame, -1, L"");
        text->LayoutFlags = "center";
        text->packSize();
        ox::gui::IGUIStaticText* screen = GUIEnvironment->addStaticText(L" ",
            ox::core::CRect<int>(0, 0, 200, 400), false, true, Frame, -1, L"");
        screen->setParagraphIcon("ScreenStatistics1",
            Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true), true);
        screen->LayoutFlags = "br";
        screen = GUIEnvironment->addStaticText(L" ", ox::core::CRect<int>(0, 0, 200, 400), false, true, Frame, -1,
            L"");
        screen->setParagraphIcon("ScreenStatistics2",
            Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true), true);
        GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), Frame, ID_CLOSE, L"Ok")->LayoutFlags =
            "center br";
    }
    Frame->sortRiver(true, 15, 15, false);
    Frame->centerOnParent();
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
