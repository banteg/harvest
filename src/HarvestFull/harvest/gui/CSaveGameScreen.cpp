// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include <algorithm>
#include "harvest/game/CWorld.h"
#include "CSaveGameScreen.h"
#include "harvest/CHarvestFullMain.h"
#include "harvest/ECustomEvents.h"
#include "harvest/game/CThreatLevel.h"
#include "harvest/settings/CHarvestProfile.h"
#include "harvest/settings/CProfileManager.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/IOxDevice.h"
#include "ox/core/CBasic.h"
#include "ox/gui/IGUIButton.h"
#include "ox/gui/IGUIEditBox.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUILayout.h"
#include "ox/gui/IGUIListBox.h"
#include "ox/gui/IGUIStaticText.h"
#include "ox/io/CHelpIO.h"
#include "ox/io/IFileList.h"
#include "ox/io/IFileSystem.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {
namespace gui {

CSaveGameScreen::CSaveGameScreen(ox::IOxDevice* device)
    : Device(device), Window(0), Frame(0), ActionButton(0), ListBox(0), Loading(false), SaveBox(0),
      DescriptionBox(0)
{
    GUIEnvironment = device->getGUIEnvironment();
    Driver = device->getVideoDriver();
    for (int i = 0; i < MODE_ICON_COUNT; ++i)
        ModeIcons[i] = 0;
    for (int i = 0; i < PLANET_ICON_COUNT; ++i)
        PlanetIcons[i] = 0;
}

CSaveGameScreen::~CSaveGameScreen()
{
    if (Window)
        Window->remove();
    for (int i = 0; i < MODE_ICON_COUNT; ++i)
        if (ModeIcons[i])
            ModeIcons[i]->remove();
    for (int i = 0; i < PLANET_ICON_COUNT; ++i)
        if (PlanetIcons[i])
            PlanetIcons[i]->remove();
}

bool CSaveGameScreen::OnEvent(const ox::event::SEvent& event)
{
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
            case ID_BACK:
                setVisible(false, Loading);
                sendCustomEvent(ECE_CONTINUE_GAME);
                return true;
            case ID_ACTION:
                performListAction();
                return true;
            case ID_DELETE:
                if (ListBox)
                {
                    int selected = ListBox->getSelected();
                    if (selected >= 0 && selected < (int)SaveList.size())
                    {
                        DeleteFilename = SaveList[selected].Filename;
                        ox::core::CString<wchar_t> question = settings::gp_systemConfig->getLocalizedText(
                            L"menu:deleteQuestion", SaveList[selected].Header.Description.c_str());
                        GUIEnvironment->addMessageBox(L"Delete game?", question.c_str(), true,
                            ox::gui::EMBF_YES | ox::gui::EMBF_NO, 0, ID_CONFIRM_DELETE);
                    }
                }
                return true;
            case ID_SAVE:
                if (SaveBox)
                {
                    if (DescriptionBox)
                        SelectedDescription = DescriptionBox->getText();
                    SaveBox->remove();
                    SaveBox = 0;
                }
                sendCustomEvent(ECE_SAVE_GAME);
                setVisible(false, Loading);
                sendCustomEvent(ECE_CONTINUE_GAME);
                return true;
            case ID_CANCEL:
                if (SaveBox)
                {
                    SaveBox->remove();
                    SaveBox = 0;
                }
                return true;
            }
            break;
        case ox::gui::EGET_LISTBOX_SELECTED_AGAIN:
            if (event.GUIEvent.Caller == ListBox)
            {
                performListAction();
                return true;
            }
            break;
        case ox::gui::EGET_MESSAGEBOX_YES:
            if (id == ID_CONFIRM_DELETE)
            {
                Device->getFileSystem()->deleteFile(DeleteFilename.c_str());
                loadSaveGames();
                return true;
            }
            if (id == ID_CONFIRM_OVERWRITE)
            {
                createSaveGameBox();
                return true;
            }
            break;
        case ox::gui::EGET_ELEMENT_DRAWN:
            switch (id)
            {
            case ID_MODE_ICON:
            case ID_MODE_ICON + 1:
            case ID_MODE_ICON + 2:
            case ID_MODE_ICON + 3:
            case ID_MODE_ICON + 4:
            {
                int index = id - ID_MODE_ICON;
                if (index >= 0 && index < MODE_ICON_COUNT && ModeIcons[index])
                {
                    ox::core::CRect<int> rect = event.GUIEvent.Caller->getAbsolutePosition();
                    ox::core::CRect<int> clip = event.GUIEvent.Caller->getAbsoluteClippingRect();
                    ox::core::CPosition2d<int> size = ModeIcons[index]->getFrameSize(0);
                    ModeIcons[index]->draw(ox::core::CPosition2d<int>(
                        (rect.LowerRightCorner.X + rect.UpperLeftCorner.X) / 2 - size.X / 2,
                        (rect.LowerRightCorner.Y + rect.UpperLeftCorner.Y) / 2 - size.Y / 2),
                        &clip, ox::video::SColor(0xffffffff));
                }
                return true;
            }
            case ID_PLANET_ICON:
            case ID_PLANET_ICON + 1:
            case ID_PLANET_ICON + 2:
            {
                int index = id - ID_PLANET_ICON;
                if (index >= 0 && index < PLANET_ICON_COUNT && PlanetIcons[index])
                {
                    ox::core::CRect<int> rect = event.GUIEvent.Caller->getAbsolutePosition();
                    ox::core::CRect<int> clip = event.GUIEvent.Caller->getAbsoluteClippingRect();
                    ox::core::CPosition2d<int> size = PlanetIcons[index]->getFrameSize(0);
                    PlanetIcons[index]->draw(ox::core::CPosition2d<int>(
                        (rect.LowerRightCorner.X + rect.UpperLeftCorner.X) / 2 - size.X / 2,
                        (rect.LowerRightCorner.Y + rect.UpperLeftCorner.Y) / 2 - size.Y / 2),
                        &clip, ox::video::SColor(0xffffffff));
                }
                return true;
            }
            }
            break;
        default:
            break;
        }
        break;
    }
    case ox::event::EET_MOUSE_INPUT_EVENT:
        return true;
    case ox::event::EET_KEY_INPUT_EVENT:
        if (event.KeyInput.Event == ox::event::EKIE_KEY_LEFT_UP && event.KeyInput.Key == ox::KEY_ESCAPE)
        {
            if (SaveBox)
            {
                SaveBox->remove();
                SaveBox = 0;
            }
            else
            {
                setVisible(false, Loading);
                sendCustomEvent(ECE_CONTINUE_GAME);
            }
        }
        return true;
    default:
        break;
    }
    return false;
}

void CSaveGameScreen::performListAction()
{
    if (!ListBox)
        return;

    int selected = ListBox->getSelected();
    ox::core::CString<char> filename;
    ox::core::CString<wchar_t> description;
    bool newSlot = true;
    if (selected >= 0 && selected < (int)SaveList.size())
    {
        filename = SaveList[selected].Filename;
        newSlot = false;
        description = SaveList[selected].Header.Description;
    }

    if (Loading)
    {
        if (!newSlot)
        {
            g_loadGameFilename = filename;
            ox::event::SEvent startEvent;
            startEvent.EventType = ox::event::EET_USER_EVENT;
            startEvent.UserEvent.UserData1 = ECE_START_GAME;
            startEvent.UserEvent.UserData2 = 0;
            startEvent.UserEvent.UserData3 = 0;
            startEvent.UserEvent.UserPointer = 0;
            ox::event::gp_subscriberList->OnEvent(startEvent);
        }
        return;
    }

    if (newSlot)
    {
        ox::core::CString<char> path("$HARVEST_USERDATA$/profiles/");
        path.append(ox::core::CString<char>("saveGame"));
        filename = ox::io::CHelpIO::getNextFreeFilename(Device->getFileSystem(), path.c_str(), ".hsg");
        description = L"";
    }
    SelectedFilename = filename;
    SelectedDescription = description;
    if (newSlot)
    {
        createSaveGameBox();
    }
    else
    {
        ox::core::CString<wchar_t> question = settings::gp_systemConfig->getLocalizedText(
            L"menu:overwriteQuestion", SaveList[selected].Header.Description.c_str());
        GUIEnvironment->addMessageBox(L"Overwrite game?", question.c_str(), true,
            ox::gui::EMBF_YES | ox::gui::EMBF_NO, 0, ID_CONFIRM_OVERWRITE);
    }
}

void CSaveGameScreen::setVisible(bool visible, bool loading)
{
    Loading = loading;
    if (visible)
    {
        if (!Window)
        {
            Window = GUIEnvironment->addModalScreen();
            ox::gui::IGUILayout* frame = GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 400, 400), Window, -1);
            ox::gui::IGUILayout* listGroup =
                (ox::gui::IGUILayout*)GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 300, 400), frame);
            ListBox = GUIEnvironment->addListBox(ox::core::CRect<int>(0, 0, 350, 450), listGroup, -1, false);
            ListBox->setSelectable(true);
            ox::gui::IGUILayout* buttonGroup =
                (ox::gui::IGUILayout*)GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 100, 400), frame);

            ActionButton = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), buttonGroup, ID_ACTION,
                settings::gp_systemConfig->getLocalizedText(L"menu:load").c_str());
            ActionButton->LayoutFlags = "br";
            ActionButton->setOverrideFont(
                GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt"));

            ox::gui::IGUIButton* deleteButton = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20),
                buttonGroup, ID_DELETE, settings::gp_systemConfig->getLocalizedText(L"menu:delete").c_str());
            deleteButton->LayoutFlags = "br";
            deleteButton->setOverrideFont(
                GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt"));

            listGroup->sortRiver(true, 5, 5, false);
            buttonGroup->sortRiver(true, 5, 5, false);

            ox::gui::IGUIButton* backButton = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), frame,
                ID_BACK, settings::gp_systemConfig->getLocalizedText(L"menu:back").c_str());
            backButton->LayoutFlags = "br right";
            backButton->setOverrideFont(
                GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt"));

            frame->sortRiver(true, 5, 5, false);
            Frame = frame;

            ox::video::ISpritePackage* package =
                Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true);
            if (package)
            {
                ModeIcons[0] = package->addNewAnimationState("ModeBtnSmallNormal");
                ModeIcons[1] = package->addNewAnimationState("ModeBtnSmallWave");
                ModeIcons[2] = package->addNewAnimationState("ModeBtnSmallInsane");
                ModeIcons[3] = package->addNewAnimationState("ModeBtnSmallRush");
                ModeIcons[4] = package->addNewAnimationState("ModeBtnSmallCreative");
                PlanetIcons[0] = package->addNewAnimationState("PlanetIcon1");
                PlanetIcons[1] = package->addNewAnimationState("PlanetIcon2");
                PlanetIcons[2] = package->addNewAnimationState("PlanetIcon3");
            }
        }
        if (Frame)
            Frame->centerOnParent();
        Window->setVisible(true);
    }
    else if (Window)
    {
        Window->setVisible(false);
    }

    if (ActionButton)
    {
        if (Loading)
            ActionButton->setText(settings::gp_systemConfig->getLocalizedText(L"menu:load").c_str());
        else
            ActionButton->setText(settings::gp_systemConfig->getLocalizedText(L"menu:save").c_str());
    }

    if (visible)
        loadSaveGames();
}

void CSaveGameScreen::loadSaveGames()
{
    if (!ListBox)
        return;

    ox::core::CString<wchar_t> playerName = settings::gp_profileManager->getCurrentProfile()->getPlayerName();
    ox::core::CString<wchar_t> playerGroup = settings::gp_profileManager->getCurrentProfile()->getPlayerGroup();
    ListBox->clear();
    SaveList.clear();

    ox::io::IFileSystem* fileSystem = Device->getFileSystem();
    ox::core::CString<char> directory("$HARVEST_USERDATA$/profiles/");
    ox::io::IFileList* files = fileSystem->createFileList("*.hsg", directory.c_str(), (ox::io::EFileList)1);
    for (int i = 0; i < files->getFileCount(); ++i)
    {
        ox::core::CString<char> filename(files->getFullFileName(i));
        ox::io::IReadFile* file = fileSystem->createAndOpenFile(filename.c_str());
        settings::SSavestateHeader header;
        if (file && settings::CSavestateInfo::readHeader(file, header))
        {
            ox::core::CString<wchar_t> name(files->getFileName(i));
            if (header.Description.size() <= 0)
                header.Description = name;
            if (header.PlayerName == playerName)
            {
                SSaveListItem item;
                item.Filename = filename;
                item.Header = header;
                item.Name = name;
                SaveList.push_back(item);
            }
        }
        if (file)
            file->drop();
    }
    files->drop();

    std::sort(SaveList.begin(), SaveList.end(), SSaveGameSorter());
    for (int i = 0; i < (int)SaveList.size(); ++i)
        addSavestateSlot(SaveList[i].Header, SaveList[i].Name);
    if (!Loading)
        addEmptySlot();
    ListBox->sortItems(false);
}

void CSaveGameScreen::createSaveGameBox()
{
    if (SaveBox)
    {
        SaveBox->remove();
        SaveBox = 0;
    }

    SaveBox = GUIEnvironment->addModalScreen();
    ox::gui::IGUILayout* frame = GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 300, 50), SaveBox, -1);
    GUIEnvironment->addStaticText(settings::gp_systemConfig->getLocalizedText(L"menu:saveDescription").c_str(),
        "br center", frame, 0, -1);

    DescriptionBox = GUIEnvironment->addEditBox(SelectedDescription.c_str(), ox::core::CRect<int>(0, 0, 250, 20),
        true, frame, -1);
    DescriptionBox->LayoutFlags = "br";
    DescriptionBox->setAssociatedButton(ID_SAVE);

    ox::gui::IGUIButton* cancelButton = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), frame,
        ID_CANCEL, settings::gp_systemConfig->getLocalizedText(L"profile:cancel").c_str());
    cancelButton->LayoutFlags = "br center";
    cancelButton->setOverrideFont(GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt"));

    ox::gui::IGUIButton* saveButton = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), frame,
        ID_SAVE, settings::gp_systemConfig->getLocalizedText(L"menu:save").c_str());
    saveButton->setOverrideFont(GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt"));

    frame->sortRiver(true, 5, 5, false);
    frame->centerOnParent();
    GUIEnvironment->setFocus(DescriptionBox);
}

bool CSaveGameScreen::isVisible()
{
    if (Window)
        return Window->isVisible();
    return false;
}

const char* CSaveGameScreen::getSelectedSaveFilename()
{
    return SelectedFilename.c_str();
}

const wchar_t* CSaveGameScreen::getSelectedSaveDescription()
{
    return SelectedDescription.c_str();
}

void CSaveGameScreen::addSavestateSlot(const settings::SSavestateHeader& header,
    const ox::core::CString<wchar_t>& name)
{
    ox::gui::IGUIElement* slot =
        GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 300, 50), ListBox->getListParent());

    ox::gui::IGUIElement* modeIcon = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 90, 50), slot);
    modeIcon->setReportOnDraw(true);
    modeIcon->setID(ID_MODE_ICON + header.GameMode);

    ox::gui::IGUIElement* planetIcon = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 50, 50), slot);
    planetIcon->setReportOnDraw(true);
    planetIcon->setID(ID_PLANET_ICON + header.Planet);

    ox::gui::IGUILayout* textGroup =
        (ox::gui::IGUILayout*)GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 200, 50), slot);

    ox::core::CString<wchar_t> text;
    if (header.GameMode == game::EGM_NORMAL || header.GameMode == game::EGM_INSANE)
    {
        text.append(L"Level ");
        text.append(header.ThreatLevel);
        text.append(L", ");
    }
    ox::core::CString<char> time = ox::core::CBasic::getTimeString(header.Time, "%H:%M %Y-%m-%d");
    text.append(time.c_str());
    text.append(L"\n");
    text.append(name);

    GUIEnvironment->addStaticText(header.Description.c_str(), 150, textGroup, 0, -1, L"");
    GUIEnvironment->addStaticText(text.c_str(), 150, textGroup,
        GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/smallFont.fnt"), -1, L"")->LayoutFlags = "br";
    textGroup->sortRiver(true, 5, 5, false);
    ((ox::gui::IGUILayout*)slot)->sortRiver(true, 0, 0, false);
}

void CSaveGameScreen::addEmptySlot()
{
    ox::gui::IGUIElement* slot =
        GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 300, 50), ListBox->getListParent());
    ox::gui::IGUIStaticText* text = GUIEnvironment->addStaticText(
        settings::gp_systemConfig->getLocalizedText(L"menu:newSlot").c_str(), ox::core::CRect<int>(0, 0, 300, 50),
        false, false, slot, -1, 0);
    text->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);
}

} // end namespace gui
} // end namespace harvest
