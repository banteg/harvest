// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "CSettingsScreen.h"
#include "harvest/ECustomEvents.h"
#include "harvest/settings/CHarvestProfile.h"
#include "harvest/settings/CProfileManager.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/IOxDevice.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/gui/IGUIButton.h"
#include "ox/gui/IGUICheckBox.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUILayout.h"
#include "ox/gui/IGUIListBox.h"
#include "ox/gui/IGUIScrollBar.h"
#include "ox/gui/IGUISkin.h"
#include "ox/gui/IGUIStaticText.h"
#include "ox/gui/IGUITabControl.h"
#include "ox/input/KeyNames.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/IVideoModeList.h"

namespace harvest {
namespace gui {

CSettingsScreen::CSettingsScreen(ox::IOxDevice* device, bool languageSelection)
    : Device(device), Window(0), LoadBlock(0), Resolutions(0), ResolutionCount(0), KeyCommand(-1)
{
    ox::gui::IGUIEnvironment* env = device->getGUIEnvironment();
    ox::core::CRect<int> buttonRect(0, 0, 90, 20);
    const ox::video::SColor white(0xffffffff);

    Window = env->addFrame(ox::core::CRect<int>(0, 0, 380, 410), env->getRootGUIElement(), -1);
    Window->setVisible(false);
    Window->centerOnParent();

    ox::gui::IGUITabControl* tabs =
        env->addTabControl(ox::core::CRect<int>(0, 0, 380, 410), Window, false, true, -1);
    tabs->setAnimations(env->getSkin()->getSpritePackage(), "Tabframe");

    // General tab
    ox::gui::IGUITab* generalTab =
        tabs->addTab(settings::gp_systemConfig->getLocalizedText(L"settings:general").c_str(), -1);
    ox::gui::IGUILayout* generalGroup = env->addLayoutGroup(ox::core::CRect<int>(0, 0, 400, 410), generalTab);
    generalGroup->LayoutFlags = "center";

    ox::gui::IGUILayout* audioGroup = env->addLayoutGroup(ox::core::CRect<int>(0, 0, 325, 85), generalGroup);
    ox::gui::IGUIStaticText* text = env->addStaticText(
        settings::gp_systemConfig->getLocalizedText(L"settings:audio").c_str(), "center br", audioGroup, 0, -1);
    text->setOverrideColor(white);
    text = env->addStaticText(
        settings::gp_systemConfig->getLocalizedText(L"settings:soundVolume").c_str(), "left br", audioGroup, 0, -1);
    text->setOverrideColor(white);
    ox::gui::IGUIScrollBar* bar = env->addScrollBar(true, ox::core::CRect<int>(0, 0, 150, 20), audioGroup, ID_SFX_VOLUME);
    bar->setMax(5);
    bar->setPos(3);
    bar->LayoutFlags = "tab";
    text = env->addStaticText(
        settings::gp_systemConfig->getLocalizedText(L"settings:musicVolume").c_str(), "left br", audioGroup, 0, -1);
    text->setOverrideColor(white);
    bar = env->addScrollBar(true, ox::core::CRect<int>(0, 0, 150, 20), audioGroup, ID_MUSIC_VOLUME);
    bar->setMax(5);
    bar->setPos(3);
    bar->LayoutFlags = "tab";
    audioGroup->sortRiver(true, 5, 5, false);
    audioGroup->LayoutFlags = "center br";

    ox::gui::IGUILayout* videoGroup = env->addLayoutGroup(ox::core::CRect<int>(0, 0, 325, 285), generalGroup);
    text = env->addStaticText(
        settings::gp_systemConfig->getLocalizedText(L"settings:video").c_str(), "center br", videoGroup, 0, -1);
    text->setOverrideColor(white);
    text = env->addStaticText(
        settings::gp_systemConfig->getLocalizedText(L"settings:resolution").c_str(), "left br", videoGroup, 0, -1);
    text->setOverrideColor(white);
    ox::gui::IGUIListBox* resolutionList =
        env->addListBox(ox::core::CRect<int>(0, 0, 245, 70), videoGroup, ID_RESOLUTION, false);
    resolutionList->LayoutFlags = "center br";
    resolutionList->setSelectable(true);

    // Only 32 bit modes of at least 800x600 are offered.
    ox::video::IVideoModeList* modes = Device->getVideoModeList();
    Resolutions = new ox::core::CDimension2d<int>[modes->getVideoModeCount()];
    int count = 0;
    for (int i = 0; i < modes->getVideoModeCount(); ++i)
    {
        if (modes->getVideoModeDepth(i) == 32 && modes->getVideoModeResolution(i).Width >= 800
            && modes->getVideoModeResolution(i).Height >= 600)
        {
            ox::core::CString<wchar_t> name;
            ox::core::CDimension2d<int> resolution = modes->getVideoModeResolution(i);
            Resolutions[count++] = resolution;
            name.append(resolution.Width);
            name.append(ox::core::CString<wchar_t>(L"x"));
            name.append(resolution.Height);
            resolutionList->addTextItem(name.c_str(), 0, white, true, true);
        }
    }
    ResolutionCount = count;

    ox::gui::IGUICheckBox* fullscreen =
        env->addCheckBox(false, ox::core::CRect<int>(0, 0, 90, 20), videoGroup, ID_FULLSCREEN, L"Fullscreen");
    fullscreen->LayoutFlags = "br center";
    text = env->addStaticText(
        settings::gp_systemConfig->getLocalizedText(L"settings:particles").c_str(), "left br", videoGroup, 0, -1);
    text->setOverrideColor(white);
    bar = env->addScrollBar(true, ox::core::CRect<int>(0, 0, 150, 20), videoGroup, ID_PARTICLES);
    bar->setMax(2);
    bar->setPos(2);
    bar->LayoutFlags = "tab";
    text = env->addStaticText(
        settings::gp_systemConfig->getLocalizedText(L"settings:scrollSpeed").c_str(), "left br", videoGroup, 0, -1);
    text->setOverrideColor(white);
    bar = env->addScrollBar(true, ox::core::CRect<int>(0, 0, 150, 20), videoGroup, ID_SCROLL_SPEED);
    bar->setMax(490);
    bar->setPos(90);
    bar->LayoutFlags = "tab";
    fullscreen->LayoutFlags = "br center";
    videoGroup->sortRiver(true, 5, 5, false);
    videoGroup->LayoutFlags = "br";

    if (languageSelection)
    {
        ox::gui::IGUILayout* languageGroup =
            env->addLayoutGroup(ox::core::CRect<int>(0, 0, 325, 285), generalGroup);
        text = env->addStaticText(settings::gp_systemConfig->getLocalizedText(L"settings:language").c_str(),
            "center br", languageGroup, 0, -1);
        text->setOverrideColor(white);
        ox::gui::IGUIListBox* languageList =
            env->addListBox(ox::core::CRect<int>(0, 0, 245, 70), languageGroup, ID_LANGUAGE, false);
        languageList->LayoutFlags = "center br";
        languageList->setSelectable(true);
        ox::TArray<settings::SLanguageFile> languages;
        settings::gp_systemConfig->getAllLanguages(languages);
        for (unsigned int i = 0; i < languages.size(); ++i)
        {
            languageList->addTextItem(languages[i].Name.c_str(), 0, white, true, true);
            if (languages[i].Name == settings::gp_systemConfig->getCurrentLanguageDisplayName())
                languageList->setSelected(i);
        }
        languageGroup->sortRiver(true, 5, 5, false);
        languageGroup->LayoutFlags = "br";
    }

    ox::gui::IGUIButton* button = env->addButton(buttonRect, Window, ID_DONE,
        settings::gp_systemConfig->getLocalizedText(L"menu:done").c_str());
    button->LayoutFlags = "right br";
    generalGroup->sortRiver(true, 5, 5, false);
    generalTab->sortRiver(false, 5, 5, false);

    // Keys tab
    ox::gui::IGUITab* keysTab =
        tabs->addTab(settings::gp_systemConfig->getLocalizedText(L"settings:keys").c_str(), -1);
    ox::gui::IGUILayout* keysGroup = env->addLayoutGroup(ox::core::CRect<int>(0, 0, 400, 400), keysTab);
    keysGroup->LayoutFlags = "center";
    ox::gui::IGUIListBox* keyList = env->addListBox(ox::core::CRect<int>(0, 0, 360, 320), keysGroup, -1, false);
    ox::gui::IGUIElement* keyListParent = keyList->getListParent();
    ox::gui::IGUILayout* keyGroup = env->addLayoutGroup(ox::core::CRect<int>(0, 0, 360, 30), keyListParent);
    for (int command = settings::EKC_INCREASE_SPEED; command < settings::EKC_COUNT; ++command)
    {
        ox::gui::IGUIStaticText* label = env->addStaticText(
            settings::gp_systemConfig->getLocalizedText(settings::KeyCommandNames[command]).c_str(), "left",
            keyGroup, 0, -1);
        ox::gui::IGUIButton* keyButton =
            env->addButton(ox::core::CRect<int>(0, 0, 100, 20), keyGroup, ID_KEY_COMMAND + command, L"Empty");
        label->LayoutFlags = "br";
        keyButton->LayoutFlags = "tab";
    }
    keyGroup->sortRiver(true, 5, 10, false);
    keyList->sortItems(false);
    button = env->addButton(buttonRect, keysGroup, ID_RESET_KEYS,
        settings::gp_systemConfig->getLocalizedText(L"settings:resetKeys").c_str());
    button->LayoutFlags = "right br";
    keysGroup->sortRiver(true, 5, 5, false);
    keysTab->sortRiver(false, 5, 5, false);

    Window->sortRiver(true, 5, 5, false);
    Window->centerOnParent();
    subscribe(ox::event::gp_subscriberList);
}

CSettingsScreen::~CSettingsScreen()
{
    delete [] Resolutions;
    if (Window)
        Window->remove();
    if (LoadBlock)
    {
        LoadBlock->remove();
        LoadBlock = 0;
    }
}

void CSettingsScreen::setVisible(bool visible)
{
    Window->setVisible(visible);
    if (visible)
        setValues();
}

void CSettingsScreen::setValues()
{
    setSliderValue(ID_SFX_VOLUME, settings::gp_systemConfig->getSfxVolume());
    setSliderValue(ID_MUSIC_VOLUME, settings::gp_systemConfig->getMusicVolume());
    setSliderValue(ID_PARTICLES, settings::gp_systemConfig->getParticleSetting());
    setSliderValue(ID_SCROLL_SPEED, (int)(settings::gp_systemConfig->getScrollSpeed() * 100.0 - 10.0));
    setCheckboxChecked(ID_FULLSCREEN, Device->getVideoDriver()->isFullscreen());
    setListSelection(ID_RESOLUTION, settings::gp_systemConfig->getScreenRes());
    LanguageSelection = getListSelection(ID_LANGUAGE);
    updateKeyMappings();
}

bool CSettingsScreen::isVisible()
{
    if (Window)
        return Window->isVisible();
    return false;
}

int CSettingsScreen::getSliderValue(int id)
{
    ox::gui::IGUIElement* element = Window->getElementFromId(id, true);
    if (element && element->getType() == ox::gui::EGUIET_SCROLL_BAR)
        return static_cast<ox::gui::IGUIScrollBar*>(element)->getPos();
    return 0;
}

bool CSettingsScreen::getCheckboxChecked(int id)
{
    ox::gui::IGUIElement* element = Window->getElementFromId(id, true);
    if (element && element->getType() == ox::gui::EGUIET_CHECK_BOX)
        return static_cast<ox::gui::IGUICheckBox*>(element)->isChecked();
    return false;
}

int CSettingsScreen::getListSelection(int id)
{
    ox::gui::IGUIElement* element = Window->getElementFromId(id, true);
    if (element && element->getType() == ox::gui::EGUIET_LIST_BOX)
        return static_cast<ox::gui::IGUIListBox*>(element)->getSelected();
    return -1;
}

void CSettingsScreen::setSliderValue(int id, int value)
{
    ox::gui::IGUIElement* element = Window->getElementFromId(id, true);
    if (element && element->getType() == ox::gui::EGUIET_SCROLL_BAR)
        static_cast<ox::gui::IGUIScrollBar*>(element)->setPos(value);
}

void CSettingsScreen::setCheckboxChecked(int id, bool checked)
{
    ox::gui::IGUIElement* element = Window->getElementFromId(id, true);
    if (element && element->getType() == ox::gui::EGUIET_CHECK_BOX)
        static_cast<ox::gui::IGUICheckBox*>(element)->setChecked(checked);
}

void CSettingsScreen::setListSelection(int id, const ox::core::CDimension2d<int>& resolution)
{
    ox::gui::IGUIElement* element = Window->getElementFromId(id, true);
    if (element && element->getType() == ox::gui::EGUIET_LIST_BOX)
    {
        for (int i = 0; i < ResolutionCount; ++i)
        {
            if (Resolutions[i] == resolution)
            {
                static_cast<ox::gui::IGUIListBox*>(element)->setSelected(i);
                return;
            }
        }
    }
}

void CSettingsScreen::updateKeyMappings()
{
    settings::CHarvestProfile* profile = settings::gp_profileManager->getCurrentProfile();
    if (!profile)
        return;
    for (int command = settings::EKC_INCREASE_SPEED; command < settings::EKC_COUNT; ++command)
    {
        ox::gui::IGUIElement* element = Window->getElementFromId(ID_KEY_COMMAND + command, true);
        if (element)
        {
            int key = profile->getKeyForCommand((settings::EKeyCommands)command);
            if (key != 0)
                element->setText(ox::input::KEY_NAMES[key]);
            else
                element->setText(L"No key set");
        }
    }
}

void CSettingsScreen::loadDefaultValues()
{
    setSliderValue(ID_SFX_VOLUME, 3);
    setSliderValue(ID_MUSIC_VOLUME, 3);
    setSliderValue(ID_PARTICLES, 2);
    setSliderValue(ID_SCROLL_SPEED, 90);
    setCheckboxChecked(ID_FULLSCREEN, false);
    setListSelection(ID_RESOLUTION, ox::core::CDimension2d<int>(800, 600));
}

void CSettingsScreen::applyScreenMode()
{
    ox::core::CDimension2d<int> resolution = Resolutions[getListSelection(ID_RESOLUTION)];
    ox::core::CDimension2d<int> current = Device->getVideoDriver()->getScreenSize();
    bool fullscreen = getCheckboxChecked(ID_FULLSCREEN);
    bool currentFullscreen = Device->getVideoDriver()->isFullscreen();
    if (resolution != current)
        Device->resizeDeviceWindow(resolution);
    if (fullscreen != currentFullscreen || resolution != current)
        Device->setFullscreenMode(fullscreen);
}

bool CSettingsScreen::OnEvent(const ox::event::SEvent& event)
{
    bool result = false;
    if (event.EventType == ox::event::EET_GUI_EVENT)
    {
        int id = event.GUIEvent.Caller->getID();
        switch (event.GUIEvent.EventType)
        {
        case ox::gui::EGET_BUTTON_CLICKED:
            if (id == ID_DONE)
            {
                settings::gp_systemConfig->saveConfig();
                Window->setVisible(false);
                sendCustomEvent(ECE_CONTINUE_GAME);
                int language = getListSelection(ID_LANGUAGE);
                if (language != LanguageSelection)
                {
                    ox::TArray<settings::SLanguageFile> languages;
                    settings::gp_systemConfig->getAllLanguages(languages);
                    settings::gp_systemConfig->openLanguageFile(languages[language].Filename);
                    sendCustomEvent(ECE_LANGUAGE_CHANGED);
                }
                result = true;
            }
            else if (id >= ID_KEY_COMMAND && id < ID_KEY_COMMAND + settings::EKC_COUNT)
            {
                if (LoadBlock)
                {
                    LoadBlock->remove();
                    LoadBlock = 0;
                }
                createLoadBlock();
                KeyCommand = id - ID_KEY_COMMAND;
                result = true;
            }
            break;
        case ox::gui::EGET_SCROLL_BAR_CHANGED:
            switch (id)
            {
            case ID_SFX_VOLUME:
                Device->getAudioDriver()->setSoundEffectVolume(getSliderValue(ID_SFX_VOLUME));
                settings::gp_systemConfig->setSfxVolume(getSliderValue(ID_SFX_VOLUME));
                result = true;
                break;
            case ID_MUSIC_VOLUME:
                Device->getAudioDriver()->setMusicVolume(getSliderValue(ID_MUSIC_VOLUME));
                settings::gp_systemConfig->setMusicVolume(getSliderValue(ID_MUSIC_VOLUME));
                result = true;
                break;
            case ID_PARTICLES:
            {
                settings::gp_systemConfig->setParticleSetting(getSliderValue(ID_PARTICLES));
                ox::event::SEvent changed;
                changed.EventType = ox::event::EET_USER_EVENT;
                changed.UserEvent.UserData1 = ECE_PARTICLE_SETTING_CHANGED;
                ox::event::gp_subscriberList->OnEvent(changed);
                result = true;
                break;
            }
            case ID_SCROLL_SPEED:
            {
                settings::gp_systemConfig->setScrollSpeed((getSliderValue(ID_SCROLL_SPEED) + 10) * 0.01f);
                ox::event::SEvent changed;
                changed.EventType = ox::event::EET_USER_EVENT;
                changed.UserEvent.UserData1 = ECE_SCROLL_SPEED_CHANGED;
                ox::event::gp_subscriberList->OnEvent(changed);
                result = true;
                break;
            }
            }
            break;
        case ox::gui::EGET_CHECKBOX_TOGGLED:
            if (id == ID_FULLSCREEN)
            {
                settings::gp_systemConfig->setFullscreen(getCheckboxChecked(ID_FULLSCREEN));
                applyScreenMode();
                result = true;
            }
            break;
        case ox::gui::EGET_LISTBOX_CHANGED:
            if (id == ID_RESOLUTION)
            {
                settings::gp_systemConfig->setScreenRes(Resolutions[getListSelection(ID_RESOLUTION)]);
                applyScreenMode();
                result = true;
            }
            break;
        }
    }
    else if (event.EventType == ox::event::EET_KEY_INPUT_EVENT)
    {
        if (KeyCommand >= 0)
        {
            if (event.KeyInput.Event == ox::event::EKIE_KEY_PRESSED_DOWN)
            {
                if (event.KeyInput.Key == ox::KEY_ESCAPE)
                {
                    KeyCommand = -1;
                    if (LoadBlock)
                    {
                        LoadBlock->remove();
                        LoadBlock = 0;
                    }
                    result = true;
                }
                else
                {
                    ox::core::CString<wchar_t> keyName(ox::input::KEY_NAMES[event.KeyInput.Key]);
                    if (keyName.size() != 0)
                    {
                        ox::gui::IGUIElement* element = Window->getElementFromId(ID_KEY_COMMAND + KeyCommand, true);
                        if (element)
                        {
                            settings::CHarvestProfile* profile = settings::gp_profileManager->getCurrentProfile();
                            if (profile && profile->makeKeyMapping(event.KeyInput.Key, (settings::EKeyCommands)KeyCommand))
                                updateKeyMappings();
                            else
                                element->setText(keyName.c_str());
                            KeyCommand = -1;
                            if (LoadBlock)
                            {
                                LoadBlock->remove();
                                LoadBlock = 0;
                            }
                            result = true;
                        }
                    }
                }
            }
        }
        else if (KeyCommand == -1 && event.KeyInput.Event == ox::event::EKIE_KEY_PRESSED_DOWN
            && event.KeyInput.Key == ox::KEY_ESCAPE && Window->isVisible())
        {
            settings::gp_systemConfig->saveConfig();
            Window->setVisible(false);
            sendCustomEvent(ECE_CONTINUE_GAME);
            result = true;
        }
    }
    else if (event.EventType == ox::event::EET_DEVICE_EVENT)
    {
        if (event.DeviceEvent.Type == ox::event::EDE_FULLSCREEN_TOGGLED)
        {
            ox::core::CDimension2d<int> size = Device->getVideoDriver()->getScreenSize();
            Window->centerOnRect(ox::core::CRect<int>(0, 0, size.Width, size.Height));
            setCheckboxChecked(ID_FULLSCREEN, Device->getVideoDriver()->isFullscreen());
        }
    }
    return result;
}

void CSettingsScreen::createLoadBlock()
{
    LoadBlock = Device->getGUIEnvironment()->addModalScreen();
    ox::gui::IGUILayout* frame =
        Device->getGUIEnvironment()->addFrame(ox::core::CRect<int>(0, 0, 30, 30), LoadBlock, -1);
    Device->getGUIEnvironment()->addStaticText(
        settings::gp_systemConfig->getLocalizedText(L"settings:pressKey").c_str(), "br center", frame, 0, -1);
    frame->sortRiver(true, 20, 20, false);
    frame->centerOnParent();
}

} // end namespace gui
} // end namespace harvest
