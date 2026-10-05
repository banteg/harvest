// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "CIngameMenuScreen.h"
#include "harvest/ECustomEvents.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/IOxDevice.h"
#include "ox/gui/IGUIButton.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUILayout.h"

namespace harvest {
namespace gui {

CIngameMenuScreen::CIngameMenuScreen(ox::IOxDevice* device)
    : Device(device), Window(0)
{
    GUIEnvironment = device->getGUIEnvironment();
    Driver = device->getVideoDriver();
}

CIngameMenuScreen::~CIngameMenuScreen()
{
    if (Window)
        Window->remove();
}

bool CIngameMenuScreen::OnEvent(const ox::event::SEvent& event)
{
    bool result = false;
    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
    {
        int id = event.GUIEvent.Caller->getID();
        switch (event.GUIEvent.EventType)
        {
        case ox::gui::EGET_BUTTON_CLICKED:
            switch (id)
            {
            case ID_NEW_GAME:
                if (Ingame)
                {
                    GUIEnvironment->addMessageBox(L"",
                        settings::gp_systemConfig->getLocalizedText(L"menu:abandonGame").c_str(), true,
                        ox::gui::EMBF_YES | ox::gui::EMBF_NO, 0, ID_CONFIRM_NEW_GAME);
                    result = true;
                    break;
                }
                setVisible(false, false, 0);
                sendCustomEvent(ECE_NEW_GAME);
                result = true;
                break;
            case ID_LOAD:
                setVisible(false, false, 0);
                sendCustomEvent(ECE_SHOW_LOAD_SCREEN);
                result = true;
                break;
            case ID_SAVE:
                setVisible(false, false, 0);
                sendCustomEvent(ECE_SHOW_SAVE_SCREEN);
                result = true;
                break;
            case ID_AWARDS:
                setVisible(false, false, 0);
                sendCustomEvent(ECE_SHOW_AWARDS_SCREEN);
                result = true;
                break;
            case ID_SETTINGS:
                setVisible(false, false, 0);
                sendCustomEvent(ECE_SHOW_SETTINGS_SCREEN);
                result = true;
                break;
            case ID_EXIT:
                if (Ingame)
                {
                    GUIEnvironment->addMessageBox(L"",
                        settings::gp_systemConfig->getLocalizedText(L"menu:abandonGame").c_str(), true,
                        ox::gui::EMBF_YES | ox::gui::EMBF_NO, 0, ID_CONFIRM_EXIT);
                    result = true;
                    break;
                }
                setVisible(false, false, 0);
                sendCustomEvent(ECE_EXIT_GAME);
                result = true;
                break;
            case ID_CONTINUE:
                setVisible(false, false, 0);
                sendCustomEvent(ECE_CONTINUE_GAME);
                result = true;
                break;
            }
            break;
        case ox::gui::EGET_MESSAGEBOX_YES:
            if (id == ID_CONFIRM_EXIT)
            {
                setVisible(false, false, 0);
                sendCustomEvent(ECE_EXIT_GAME);
                result = true;
            }
            else if (id == ID_CONFIRM_NEW_GAME)
            {
                setVisible(false, false, 0);
                sendCustomEvent(ECE_NEW_GAME);
                result = true;
            }
            break;
        default:
            break;
        }
        break;
    }
    case ox::event::EET_MOUSE_INPUT_EVENT:
        result = true;
        break;
    case ox::event::EET_KEY_INPUT_EVENT:
        result = true;
        if (event.KeyInput.Event == ox::event::EKIE_KEY_PRESSED_DOWN && event.KeyInput.Key == ox::KEY_ESCAPE)
        {
            setVisible(false, false, 0);
            sendCustomEvent(ECE_CONTINUE_GAME);
        }
        break;
    default:
        break;
    }
    return result;
}

void CIngameMenuScreen::setVisible(bool visible, bool ingame, int gameMode)
{
    Ingame = ingame;
    if (visible)
    {
        if (!Window)
        {
            Window = GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 300, 300), 0, -1);
            const wchar_t* BUTTON_TEXTS[] = {L"menu:newGame", L"menu:load", L"menu:save", L"menu:settings",
                L"menu:awards", L"menu:exit", L"menu:continue"};
            for (int id = ID_NEW_GAME; id <= ID_CONTINUE; ++id)
            {
                int index = id - ID_NEW_GAME;
                ox::core::CString<wchar_t> text = settings::gp_systemConfig->getLocalizedText(BUTTON_TEXTS[index]);
                if (id == ID_CONTINUE)
                    GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 90, 10), Window)->LayoutFlags = "br";
                ox::gui::IGUIButton* button =
                    GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), Window, id, text.c_str());
                button->LayoutFlags = "center br";
                if (index == ID_SAVE - ID_NEW_GAME && gameMode == 5)
                    button->setEnabled(false);
                button->setOverrideFont(
                    GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt"));
            }
            Window->sortRiver(true, 5, 5, false);
        }
        Window->centerOnParent();
        Window->setVisible(true);
    }
    else if (Window)
        Window->setVisible(false);
}

bool CIngameMenuScreen::isVisible()
{
    if (Window)
        return Window->isVisible();
    return false;
}

} // end namespace gui
} // end namespace harvest
