// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "CProfileScreen.h"
#include "harvest/ECustomEvents.h"
#include "harvest/settings/CHarvestProfile.h"
#include "harvest/settings/CProfileManager.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/IOxDevice.h"
#include "ox/gui/IGUIButton.h"
#include "ox/gui/IGUIEditBox.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUILayout.h"
#include "ox/gui/IGUIListBox.h"

namespace harvest {
namespace gui {

CProfileScreen::CProfileScreen(ox::IOxDevice* device)
    : Device(device), Window(0), EditMode(false), SelectedIndex(-1)
{
    GUIEnvironment = device->getGUIEnvironment();
    Driver = device->getVideoDriver();
}

CProfileScreen::~CProfileScreen()
{
    if (Window)
        Window->remove();
}

bool CProfileScreen::OnEvent(const ox::event::SEvent& event)
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
            case ID_CREATE_PROFILE:
                createEditProfileWindow(true);
                result = true;
                break;
            case ID_EDIT_PROFILE:
                createEditProfileWindow(false);
                result = true;
                break;
            case ID_DELETE_PROFILE:
                GUIEnvironment->addMessageBox(
                    settings::gp_systemConfig->getLocalizedText(L"profile:confirmDeleteTitle").c_str(),
                    settings::gp_systemConfig->getLocalizedText(L"profile:confirmDeleteText").c_str(),
                    true, ox::gui::EMBF_YES | ox::gui::EMBF_NO, Window, ID_CONFIRM_DELETE);
                result = true;
                break;
            case ID_BACK:
                removeWindow();
                result = true;
                break;
            case ID_SAVE:
                if (saveProfile(false) && !EditMode)
                {
                    ox::event::SEvent created;
                    created.EventType = ox::event::EET_USER_EVENT;
                    created.UserEvent.UserData1 = ECE_PROFILE_CREATED;
                    ox::event::gp_subscriberList->postDelayedEvent(created);
                    removeWindow();
                }
                result = true;
                break;
            case ID_CANCEL:
                removeWindow();
                createProfileListWindow();
                result = true;
                break;
            case ID_GROUP_INFO:
                GUIEnvironment->addMessageBox(
                    settings::gp_systemConfig->getLocalizedText(L"profile:groupInfoTitle").c_str(),
                    settings::gp_systemConfig->getLocalizedText(L"profile:groupInfoText").c_str(),
                    true, ox::gui::EMBF_OK, 0, -1);
                result = true;
                break;
            }
            break;
        case ox::gui::EGET_LISTBOX_CHANGED:
            if (id == ID_PROFILE_LIST)
            {
                ox::gui::IGUIListBox* list = (ox::gui::IGUIListBox*)event.GUIEvent.Caller;
                int selected = list->getSelected();
                if (selected != SelectedIndex)
                {
                    SelectedIndex = selected;
                    ox::core::CString<wchar_t> name = list->getListItem(selected)->getText();
                    settings::gp_profileManager->openProfileByName(name);
                }
                result = true;
                break;
            }
            break;
        case ox::gui::EGET_LISTBOX_SELECTED_AGAIN:
            if (id == ID_PROFILE_LIST)
            {
                removeWindow();
                result = true;
                break;
            }
            break;
        case ox::gui::EGET_MESSAGEBOX_YES:
            switch (id)
            {
            case ID_NAME_TAKEN:
                saveProfile(true);
                result = true;
                break;
            case ID_CONFIRM_DELETE:
            {
                deleteCurrentProfile();
                ox::gui::IGUIListBox* list = (ox::gui::IGUIListBox*)Window->getElementFromId(ID_PROFILE_LIST, true);
                int selected = list->getSelected();
                if (selected != SelectedIndex)
                {
                    SelectedIndex = selected;
                    ox::core::CString<wchar_t> name = list->getListItem(selected)->getText();
                    settings::gp_profileManager->openProfileByName(name);
                }
                result = true;
                break;
            }
            }
            break;
        default:
            break;
        }
        break;
    }
    case ox::event::EET_MOUSE_INPUT_EVENT:
        switch (event.MouseInput.Event)
        {
        case ox::event::EMIE_LMOUSE_PRESSED_DOWN:
        case ox::event::EMIE_LMOUSE_LEFT_UP:
            return true;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return result;
}

void CProfileScreen::createEditProfileWindow(bool newProfile)
{
    EditMode = !newProfile;
    removeWindow();
    Window = GUIEnvironment->addModalScreen();
    ox::gui::IGUILayout* frame = GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 300, 500), Window, -1);

    ox::core::CString<wchar_t> title;
    if (settings::gp_profileManager->getCurrentProfile())
    {
        if (newProfile)
            title = settings::gp_systemConfig->getLocalizedText(L"profile:newProfileTitle");
        else
            title = settings::gp_systemConfig->getLocalizedText(L"profile:editProfileTitle");
    }
    else
        title = settings::gp_systemConfig->getLocalizedText(L"profile:newProfileFirstTitle");
    GUIEnvironment->addStaticText(title.c_str(), "br center", frame,
        GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/largeFont.fnt"), -1);

    ox::gui::IGUILayout* group =
        (ox::gui::IGUILayout*)GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 200, 400), frame);
    group->LayoutFlags = "br";
    ox::core::CRect<int> buttonRect(0, 0, 90, 20);
    ox::core::CRect<int> editRect(0, 0, 120, 15);

    GUIEnvironment->addStaticText(settings::gp_systemConfig->getLocalizedText(L"profile:enterName").c_str(),
        "br left", group, 0, -1);
    ox::gui::IGUIEditBox* nameBox = GUIEnvironment->addEditBox(L"", editRect, true, group, ID_NAME);
    nameBox->setMax(16);
    nameBox->LayoutFlags = "tab";
    nameBox->setAssociatedButton(ID_SAVE);

    GUIEnvironment->addStaticText(settings::gp_systemConfig->getLocalizedText(L"profile:enterGroup").c_str(),
        "br left", group, 0, -1);
    ox::gui::IGUIEditBox* groupBox = GUIEnvironment->addEditBox(L"", editRect, true, group, ID_GROUP);
    groupBox->setMax(16);
    groupBox->LayoutFlags = "tab";
    groupBox->setAssociatedButton(ID_SAVE);

    GUIEnvironment->addButton(buttonRect, group, ID_GROUP_INFO, L"?");
    if (settings::gp_profileManager->getCurrentProfile())
    {
        ox::gui::IGUIButton* cancel = GUIEnvironment->addButton(buttonRect, group, ID_CANCEL,
            settings::gp_systemConfig->getLocalizedText(L"profile:cancel").c_str());
        cancel->LayoutFlags = "br center";
    }
    ox::gui::IGUIButton* save = GUIEnvironment->addButton(buttonRect, group, ID_SAVE,
        settings::gp_systemConfig->getLocalizedText(L"profile:create").c_str());
    if (!settings::gp_profileManager->getCurrentProfile())
        save->LayoutFlags = "br center";
    group->sortRiver(true, 5, 5, false);
    group->centerOnParent();

    if (!newProfile)
    {
        settings::CHarvestProfile* profile = settings::gp_profileManager->getCurrentProfile();
        nameBox->setText(profile->getPlayerName().c_str());
        groupBox->setText(profile->getPlayerGroup().c_str());
        save->setText(settings::gp_systemConfig->getLocalizedText(L"profile:save").c_str());
    }

    frame->sortRiver(true, 0, 0, false);
    frame->centerOnParent();
    GUIEnvironment->setFocus(nameBox);
}

void CProfileScreen::removeWindow()
{
    if (Window)
    {
        Window->remove();
        Window = 0;
    }
}

bool CProfileScreen::saveProfile(bool overwrite)
{
    bool saved = false;
    ox::core::CString<wchar_t> name = getText(ID_NAME);
    stripIllegalCharacters(name);
    ox::core::CString<wchar_t> group = getText(ID_GROUP);
    stripIllegalCharacters(group);

    if (checkValidName(name))
    {
        ox::TArray<settings::SProfileName*>& profiles = settings::gp_profileManager->getProfileNames(false);
        for (ox::TArray<settings::SProfileName*>::iterator it = profiles.begin(); ; ++it)
        {
            if (it == profiles.end())
            {
                if (EditMode)
                    settings::gp_profileManager->getCurrentProfile()->setPlayerName(name);
                else
                    settings::gp_profileManager->createProfile(name);
                settings::gp_profileManager->getCurrentProfile()->setPlayerGroup(group);
                settings::gp_profileManager->writeCurrentProfile();
                createProfileListWindow();
                saved = true;
                break;
            }
            if ((*it)->Name == name)
            {
                if (overwrite || (EditMode && settings::gp_profileManager->getCurrentProfile()
                    && settings::gp_profileManager->getCurrentProfile()->getPlayerName() == name))
                {
                    settings::gp_profileManager->openProfile((*it)->Filename);
                    settings::gp_profileManager->getCurrentProfile()->setPlayerGroup(group);
                    settings::gp_profileManager->writeCurrentProfile();
                    createProfileListWindow();
                    saved = true;
                }
                else
                {
                    GUIEnvironment->addMessageBox(
                        settings::gp_systemConfig->getLocalizedText(L"profile:nameTakenTitle").c_str(),
                        settings::gp_systemConfig->getLocalizedText(L"profile:nameTakenText").c_str(),
                        true, ox::gui::EMBF_YES | ox::gui::EMBF_NO, 0, ID_NAME_TAKEN);
                }
                break;
            }
        }
    }
    return saved;
}

void CProfileScreen::createProfileListWindow()
{
    removeWindow();
    Window = GUIEnvironment->addModalScreen();
    ox::gui::IGUILayout* frame = GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 300, 500), Window, -1);
    frame->centerOnParent();
    ox::gui::IGUILayout* content =
        (ox::gui::IGUILayout*)GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 300, 600), frame);
    ox::gui::IGUILayout* listGroup =
        (ox::gui::IGUILayout*)GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 300, 600), content);
    GUIEnvironment->addStaticText(settings::gp_systemConfig->getLocalizedText(L"profile:selectProfile").c_str(),
        "br", listGroup, 0, -1);
    ox::gui::IGUIListBox* list =
        GUIEnvironment->addListBox(ox::core::CRect<int>(0, 0, 200, 150), listGroup, ID_PROFILE_LIST, false);
    list->setSelectable(true);
    list->LayoutFlags = "br";

    ox::TArray<settings::SProfileName*>& profiles = settings::gp_profileManager->getProfileNames(true);
    ox::TArray<settings::SProfileName*>::iterator it = profiles.begin();
    ox::core::CString<wchar_t> selected;
    if (!settings::gp_profileManager->getCurrentProfile())
    {
        selected = profiles[0]->Name;
        settings::gp_profileManager->openProfile(profiles[0]->Filename);
    }
    else
        selected = settings::gp_profileManager->getCurrentProfile()->getPlayerName();
    for (; it != profiles.end(); ++it)
    {
        list->addTextItem((*it)->Name.c_str(), 0, ox::video::SColor(0xffffffff), true, true);
        if ((*it)->Name == selected)
            list->setSelected(list->getItemCount() - 1);
    }
    listGroup->sortRiver(true, 5, 5, false);

    ox::gui::IGUILayout* buttons =
        (ox::gui::IGUILayout*)GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 300, 600), content);
    ox::core::CRect<int> buttonRect(0, 0, 90, 20);
    ox::gui::IGUIButton* button = GUIEnvironment->addButton(buttonRect, buttons, ID_CREATE_PROFILE,
        settings::gp_systemConfig->getLocalizedText(L"profile:createProfile").c_str());
    button->LayoutFlags = "br";
    button = GUIEnvironment->addButton(buttonRect, buttons, ID_EDIT_PROFILE,
        settings::gp_systemConfig->getLocalizedText(L"profile:editProfile").c_str());
    button->LayoutFlags = "br";
    button = GUIEnvironment->addButton(buttonRect, buttons, ID_DELETE_PROFILE,
        settings::gp_systemConfig->getLocalizedText(L"profile:deleteProfile").c_str());
    button->LayoutFlags = "br";
    buttons->sortRiver(true, 5, 5, false);
    button = GUIEnvironment->addButton(buttonRect, content, ID_BACK,
        settings::gp_systemConfig->getLocalizedText(L"profile:back").c_str());
    button->LayoutFlags = "br right";
    content->sortRiver(true, 5, 5, false);
    content->centerOnParent();
    frame->sortRiver(true, 0, 0, false);
    frame->centerOnParent();
}

void CProfileScreen::deleteCurrentProfile()
{
    if (!Window)
        return;

    ox::gui::IGUIListBox* list = (ox::gui::IGUIListBox*)Window->getElementFromId(ID_PROFILE_LIST, true);
    if (list->getItemCount() == 1)
    {
        GUIEnvironment->addMessageBox(
            settings::gp_systemConfig->getLocalizedText(L"profile:deleteFailedTitle").c_str(),
            settings::gp_systemConfig->getLocalizedText(L"profile:deleteFailedText").c_str(),
            true, ox::gui::EMBF_OK, GUIEnvironment->getRootGUIElement(), -1);
        return;
    }

    settings::gp_profileManager->deleteCurrentProfile();
    ox::TArray<settings::SProfileName*> profiles = settings::gp_profileManager->getProfileNames(true);
    ox::core::CString<wchar_t> selected = profiles[0]->Name;
    settings::gp_profileManager->openProfile(profiles[0]->Filename);
    list->clear();
    for (ox::TArray<settings::SProfileName*>::iterator it = profiles.begin(); it != profiles.end(); ++it)
    {
        list->addTextItem((*it)->Name.c_str(), 0, ox::video::SColor(0xffffffff), true, true);
        if ((*it)->Name == selected)
            list->setSelected(list->getItemCount() - 1);
    }
}

bool CProfileScreen::isVisible()
{
    if (Window)
        return Window->isVisible();
    return false;
}

void CProfileScreen::stripIllegalCharacters(ox::core::CString<wchar_t>& text)
{
    text.replace(L';', L'_');
    const wchar_t* chars = text.c_str();
    int start = -1;
    do
        ++start;
    while (chars[start] == L' ');
    int end = text.size();
    do
        --end;
    while (chars[end] == L' ');
    text = text.subString(start, end - start + 1);
}

bool CProfileScreen::checkValidName(ox::core::CString<wchar_t>& name)
{
    bool valid = true;
    if (name.size() < 3)
    {
        GUIEnvironment->addMessageBox(
            settings::gp_systemConfig->getLocalizedText(L"profile:invalidNameTitle").c_str(),
            settings::gp_systemConfig->getLocalizedText(L"profile:invalidNameText").c_str(),
            true, ox::gui::EMBF_OK, 0, -1);
        valid = false;
    }
    return valid;
}

const wchar_t* CProfileScreen::getText(int id)
{
    ox::gui::IGUIElement* element = GUIEnvironment->getRootGUIElement()->getElementFromId(id, true);
    if (element)
        return element->getText();
    return L"Error!";
}

} // end namespace gui
} // end namespace harvest
