// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include <math.h>
#include "harvest/game/CWorld.h"
#include "CAchievementsScreen.h"
#include "harvest/settings/CProfileManager.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/IOxDevice.h"
#include "ox/algo/CRand.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/core/CBasic.h"
#include "ox/gui/IGUIButton.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIImage.h"
#include "ox/gui/IGUIModalScreen.h"
#include "ox/gui/IGUISkin.h"
#include "ox/gui/IGUIStaticText.h"
#include "ox/gui/IGUIWindow.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {
namespace gui {

//! Board positions of the award centers, in AwardAreas order.
static const int AWARD_POS_X[] =
{
    663, 265, 440, 350, 567, 648, 129, 47, 126, 47, 349, 314, 474, 219, 154, 571, 647, 538, 601, 31,
    376, 284, 410, 93, 349, 463, 528, 587, 23, 70, 112, 579, 619, 668, 100, 164, 232, 295, 347, 400,
    294, 347, 400, 24, 71, 114, 579, 619, 667
};

static const int AWARD_POS_Y[] =
{
    194, 46, 46, 46, 465, 450, 464, 450, 32, 46, 306, 413, 269, 271, 244, 35, 54, 243, 217, 196,
    413, 294, 293, 217, 208, 326, 302, 274, 119, 110, 85, 90, 121, 130, 274, 301, 325, 464, 470, 464,
    97, 118, 97, 366, 374, 406, 406, 373, 365
};

//! Text keys of the Medusa dialog of each earned award, in AwardAreas order.
static const wchar_t* const ACHIEVEMENT_MEDUSA_INFO[] =
{
    L"achievementMedusa:flawlessEstablishment",
    L"achievementMedusa:skilledColonisation",
    L"achievementMedusa:masterfulColonisation",
    L"achievementMedusa:supremeColonisation",
    L"achievementMedusa:ironWill",
    L"achievementMedusa:firmDefense",
    L"achievementMedusa:strategicPlanning",
    L"achievementMedusa:efficientStrategy",
    L"achievementMedusa:insaneTactics",
    L"achievementMedusa:superHumanTactics",
    L"achievementMedusa:dogfight",
    L"achievementMedusa:bombLaunch",
    L"achievementMedusa:masterIndustry",
    L"achievementMedusa:pureBlood",
    L"achievementMedusa:hero",
    L"achievementMedusa:exploration",
    L"achievementMedusa:excursion",
    L"achievementMedusa:timeIsMoney",
    L"achievementMedusa:goldenWeb",
    L"achievementMedusa:massiveEncounter",
    L"achievementMedusa:masterBlaster",
    L"achievementMedusa:humoungusLaserBeam",
    L"achievementMedusa:phoenix",
    L"achievementMedusa:perfectWave",
    L"achievementMedusa:wickedAwesome",
    L"achievementMedusa:firstHarvest1",
    L"achievementMedusa:firstHarvest2",
    L"achievementMedusa:firstHarvest3",
    L"achievementMedusa:firstKill1",
    L"achievementMedusa:firstKill2",
    L"achievementMedusa:firstKill3",
    L"achievementMedusa:firstExploration1",
    L"achievementMedusa:firstExploration2",
    L"achievementMedusa:firstExploration3",
    L"achievementMedusa:firstSpark1",
    L"achievementMedusa:firstSpark2",
    L"achievementMedusa:firstSpark3",
    L"achievementMedusa:firstBomb1",
    L"achievementMedusa:firstBomb2",
    L"achievementMedusa:firstBomb3",
    L"achievementMedusa:firstLevels1",
    L"achievementMedusa:firstLevels2",
    L"achievementMedusa:firstLevels3",
    L"achievementMedusa:firstWave1",
    L"achievementMedusa:firstWave2",
    L"achievementMedusa:firstWave3",
    L"achievementMedusa:firstDamage1",
    L"achievementMedusa:firstDamage2",
    L"achievementMedusa:firstDamage3"
};

//! The award that ends the main achievements; its Medusa dialog is a scripted call.
static const int WICKED_AWESOME = settings::ACHIEVEMENT_MAIN_COUNT - 1;

CAchievementsScreen::CAchievementsScreen(ox::IOxDevice* device)
    : Device(device), Window(0), Time(0), Hovered(-1), SelectorScale(0), MedusaWindow(0)
{
    GUIEnvironment = device->getGUIEnvironment();
    Driver = device->getVideoDriver();
    for (int i = 0; i < settings::ACHIEVEMENT_MAIN_COUNT; ++i)
        MainAwards[i] = 0;
    for (int i = 0; i < settings::ACHIEVEMENT_MINI_COUNT * 3; ++i)
        MiniAwards[i] = 0;
    for (int i = 0; i < AWARD_COUNT; ++i)
    {
        AwardAreas[i] = 0;
        Achieved[i] = false;
    }
    for (int i = 0; i < 1; ++i)
        Selector[i] = 0;
}

CAchievementsScreen::~CAchievementsScreen()
{
    if (MedusaWindow)
        MedusaWindow->remove();
    if (Window)
        Window->remove();
    for (int i = 0; i < settings::ACHIEVEMENT_MAIN_COUNT; ++i)
        if (MainAwards[i])
            MainAwards[i]->remove();
    for (int i = 0; i < settings::ACHIEVEMENT_MINI_COUNT * 3; ++i)
        if (MiniAwards[i])
            MiniAwards[i]->remove();
    for (int i = 0; i < 1; ++i)
        if (Selector[i])
            Selector[i]->remove();
}

void CAchievementsScreen::update(float time)
{
    Time += time;
    SelectorScale = ox::core::min_(time * 10.0f + SelectorScale, 1.2f);
}

bool CAchievementsScreen::OnEvent(const ox::event::SEvent& event)
{
    bool result = true;
    switch (event.EventType)
    {
    case ox::event::EET_GUI_EVENT:
    {
        int id = event.GUIEvent.Caller->getID();
        switch (event.GUIEvent.EventType)
        {
        case ox::gui::EGET_MODAL_SCREEN_BLOCKED:
            OnEvent(((ox::gui::IGUIModalScreen*)event.GUIEvent.Caller)->getLastBlockedEvent());
            break;
        case ox::gui::EGET_ELEMENT_DRAWN:
            result = false;
            if (id == ID_AWARDS)
            {
                for (int i = 0; i < AWARD_COUNT; ++i)
                {
                    if (i == WICKED_AWESOME)
                        continue;
                    if (!AwardAreas[i])
                        continue;
                    if (Hovered == i && Selector[0])
                    {
                        ox::core::CRect<int> rect = AwardAreas[i]->getAbsolutePosition();
                        Selector[0]->drawScaled(ox::core::CPosition2d<float>(
                            (rect.UpperLeftCorner.X + rect.LowerRightCorner.X) / 2,
                            (rect.UpperLeftCorner.Y + rect.LowerRightCorner.Y) / 2),
                            SelectorScale, ox::video::SColor(0xffffffff));
                    }
                    ox::video::SColor color(Achieved[i] ? 0xffffffff : 0xff000000);
                    ox::video::ISpriteAnimationState* award;
                    if (i < settings::ACHIEVEMENT_MAIN_COUNT)
                        award = MainAwards[i];
                    else
                        award = MiniAwards[i - settings::ACHIEVEMENT_MAIN_COUNT];
                    if (award)
                        award->draw(AwardAreas[i]->getAbsolutePosition().UpperLeftCorner, 0, color);
                }

                if (AwardAreas[WICKED_AWESOME])
                {
                    if (Hovered == WICKED_AWESOME && Selector[0])
                    {
                        ox::core::CRect<int> rect = AwardAreas[WICKED_AWESOME]->getAbsolutePosition();
                        Selector[0]->drawScaled(ox::core::CPosition2d<float>(
                            (rect.UpperLeftCorner.X + rect.LowerRightCorner.X) / 2,
                            (rect.UpperLeftCorner.Y + rect.LowerRightCorner.Y) / 2),
                            SelectorScale, ox::video::SColor(0xffffffff));
                    }
                    ox::video::SColor color(Achieved[WICKED_AWESOME] ? 0xffffffff : 0xff000000);
                    ox::video::ISpriteAnimationState* award = MainAwards[WICKED_AWESOME];
                    if (award)
                    {
                        ox::gui::IGUIElement* area = AwardAreas[WICKED_AWESOME];
                        float rotation = sin(Time * 5) * 0.16f + cos(Time * 9) * 0.12f + sin(Time * 13) * 0.08f +
                            cos(Time * 17) * 0.04f;
                        float scale = 1.0 + sin(Time * 7) * 0.07f + cos(Time * 11) * 0.05f + sin(Time * 17) * 0.03f +
                            cos(Time * 19) * 0.01f;
                        ox::core::CRect<int> rect = area->getAbsolutePosition();
                        award->drawRotated(ox::core::CPosition2d<float>(
                            (rect.UpperLeftCorner.X + rect.LowerRightCorner.X) / 2,
                            (rect.UpperLeftCorner.Y + rect.LowerRightCorner.Y) / 2),
                            rotation, scale, color);
                    }
                }
            }
            break;
        case ox::gui::EGET_BUTTON_CLICKED:
            switch (id)
            {
            case ID_MEDUSA_CLOSE:
                if (MedusaWindow)
                {
                    MedusaWindow->remove();
                    MedusaWindow = 0;
                }
                break;
            case ID_CLOSE:
            {
                setVisible(false);
                ox::event::SEvent closeEvent;
                closeEvent.EventType = ox::event::EET_USER_EVENT;
                closeEvent.UserEvent.UserData1 = 4;
                closeEvent.UserEvent.UserData2 = 0;
                closeEvent.UserEvent.UserData3 = 0;
                closeEvent.UserEvent.UserPointer = 0;
                ox::event::gp_subscriberList->OnEvent(closeEvent);
                break;
            }
            default:
                result = false;
            }
            break;
        default:
            result = false;
        }
        break;
    }
    case ox::event::EET_MOUSE_INPUT_EVENT:
    {
        ox::core::CPosition2d<int> mouse(event.MouseInput.X, event.MouseInput.Y);
        switch (event.MouseInput.Event)
        {
        case ox::event::EMIE_LMOUSE_PRESSED_DOWN:
            if (Hovered >= 0 && AwardAreas[Hovered] &&
                AwardAreas[Hovered]->getAbsolutePosition().isPointInside(mouse) && !MedusaWindow)
            {
                if (!Achieved[Hovered])
                {
                    Device->getAudioDriver()->playSound("BtnDenial.ogg", 1.0f, 0.0f, 1.0f);
                }
                else if (Hovered == WICKED_AWESOME)
                {
                    MedusaWindow = GUIEnvironment->addWindow(ox::core::CRect<int>(0, 0, 400, 300), true, 0, 0, -1);
                    GUIEnvironment->addStaticText(
                        settings::gp_systemConfig->getLocalizedText(L"achievements:wickedAwesome").c_str(), "br center",
                        MedusaWindow, GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/largeFont.fnt"), -1);
                    ox::gui::IGUIStaticText* text = GUIEnvironment->addStaticText(
                        settings::gp_systemConfig->getLocalizedText(L"achievementMedusa:wickedAwesome1").c_str(), 500,
                        MedusaWindow, 0, -1, L"");
                    text->LayoutFlags = "br";
                    text->setParagraphIcon("PortraitSyndicate",
                        Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", false), true);
                    text->activateProgressiveReveal(4500);
                    text = GUIEnvironment->addStaticText(
                        settings::gp_systemConfig->getLocalizedText(L"achievementMedusa:wickedAwesome2").c_str(), 500,
                        MedusaWindow, 0, -1, L"");
                    text->LayoutFlags = "br";
                    text->setParagraphIcon("OxeyeLoading|right",
                        Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true), true);
                    text->activateProgressiveReveal(25000);
                    unsigned int reveal = text->getTotalProgressiveTime() + 25000;
                    text = GUIEnvironment->addStaticText(
                        settings::gp_systemConfig->getLocalizedText(L"achievementMedusa:wickedAwesome3").c_str(), 500,
                        MedusaWindow, 0, -1, L"");
                    text->LayoutFlags = "br";
                    text->activateProgressiveReveal(reveal);
                    text = GUIEnvironment->addStaticText(
                        settings::gp_systemConfig->getLocalizedText(L"achievementMedusa:wickedAwesome4").c_str(), 500,
                        MedusaWindow, GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/largeFont.fnt"),
                        -1, L"");
                    text->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_TOP);
                    text->LayoutFlags = "br";
                    ox::gui::IGUIButton* close = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20),
                        MedusaWindow, ID_MEDUSA_CLOSE,
                        settings::gp_systemConfig->getLocalizedText(L"achievementMedusa:close").c_str());
                    close->LayoutFlags = "br";
                    MedusaWindow->sortRiver(true, 10, 15, false);
                    MedusaWindow->centerOnParent();
                    Device->getAudioDriver()->playVoice("medusa3_phone_line.ogg");
                }
                else
                {
                    MedusaWindow = GUIEnvironment->addWindow(ox::core::CRect<int>(0, 0, 400, 300), true, 0, 0, -1);
                    ox::algo::CRand rand(Hovered);
                    int file = rand.nextInt(4000);
                    GUIEnvironment->addStaticText(
                        settings::gp_systemConfig->getLocalizedText(L"achievementMedusa:file", file + 50).c_str(),
                        "br center", MedusaWindow, 0, -1);
                    ox::gui::IGUIStaticText* info = GUIEnvironment->addStaticText(
                        settings::gp_systemConfig->getLocalizedText(ACHIEVEMENT_MEDUSA_INFO[Hovered]).c_str(),
                        400, MedusaWindow, 0, -1, L"");
                    info->LayoutFlags = "br";
                    ox::core::CString<char> icon("");
                    if (Hovered < settings::ACHIEVEMENT_MAIN_COUNT)
                        icon = settings::CHarvestProfile::getAchievementSpriteName(Hovered, 0);
                    else
                        icon = settings::CHarvestProfile::getAchievementSpriteName(
                            (Hovered - settings::ACHIEVEMENT_MAIN_COUNT) / 3 + settings::ACHIEVEMENT_MAIN_COUNT,
                            (Hovered - settings::ACHIEVEMENT_MAIN_COUNT) % 3);
                    info->setParagraphIcon(icon.c_str(),
                        Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true), true);
                    ox::gui::IGUIButton* close = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20),
                        MedusaWindow, ID_MEDUSA_CLOSE,
                        settings::gp_systemConfig->getLocalizedText(L"achievementMedusa:close").c_str());
                    close->LayoutFlags = "br";
                    MedusaWindow->sortRiver(true, 10, 15, false);
                    MedusaWindow->centerOnParent();
                }
            }
            break;
        case ox::event::EMIE_MOUSE_MOVED:
        {
            int previous = Hovered;
            Hovered = -1;
            if (!MedusaWindow)
            {
                for (int i = AWARD_COUNT - 1; i >= 0; --i)
                {
                    if (AwardAreas[i] && AwardAreas[i]->getAbsolutePosition().isPointInside(mouse))
                    {
                        Hovered = i;
                        break;
                    }
                }
            }
            if (Hovered != previous)
            {
                SelectorScale = 0;
                if (Hovered >= 0 && Achieved[Hovered])
                    Device->getAudioDriver()->playSound("SelectBuilding.ogg", 1.0f, 0.0f, 1.0f);
            }
            break;
        }
        }
        break;
    }
    case ox::event::EET_KEY_INPUT_EVENT:
        break;
    default:
        result = false;
    }
    return result;
}

void CAchievementsScreen::setVisible(bool visible)
{
    if (visible)
    {
        if (Window)
            return;
        Window = GUIEnvironment->addModalScreen();
        Frame = GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 600, 500), Window, -1);
        ox::video::ISpritePackage* sprites =
            Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true);
        for (int i = 0; i < settings::ACHIEVEMENT_MAIN_COUNT; ++i)
            if (!MainAwards[i])
                MainAwards[i] = sprites->addNewAnimationState(
                    settings::CHarvestProfile::getAchievementSpriteName(i, 0));
        for (int i = 0; i < settings::ACHIEVEMENT_MINI_COUNT; ++i)
            for (int planet = 0; planet < 3; ++planet)
                if (!MiniAwards[i * 3 + planet])
                    MiniAwards[i * 3 + planet] = sprites->addNewAnimationState(
                        settings::CHarvestProfile::getAchievementSpriteName(i + settings::ACHIEVEMENT_MAIN_COUNT, planet));

        int score = settings::gp_profileManager->getCurrentProfile()->getAchievementScore();
        ox::core::CString<wchar_t> title = settings::gp_systemConfig->getLocalizedText(
            settings::ACHIEVEMENT_RATING_NAMES[settings::gp_profileManager->getCurrentProfile()->getAchievementRating(score)]);
        title.append(ox::core::CString<wchar_t>(L" ("));
        title.append(score);
        title.append(ox::core::CString<wchar_t>(L")"));
        ox::gui::IGUIStaticText* text = GUIEnvironment->addStaticText(title.c_str(), 600, Frame,
            GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/largeFont.fnt"), -1, L"");
        text->setOverrideColor(ox::video::SColor(0xffffffff));
        text->LayoutFlags = "br center";
        text->packSize();

        ox::gui::IGUILayout* awards = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 700, 460), Frame);
        awards->LayoutFlags = "br center";
        awards->setID(ID_AWARDS);
        awards->setReportOnDraw(2);

        int index;
        for (index = 0; index < settings::ACHIEVEMENT_MAIN_COUNT; ++index)
        {
            ox::core::CPosition2d<int> size(60, 60);
            if (MainAwards[index])
                size = MainAwards[index]->getFrameSize(0);
            int y = AWARD_POS_Y[index] - size.Y / 2;
            int x = AWARD_POS_X[index] - size.X / 2;
            if (y > 340)
                y -= 40;
            AwardAreas[index] = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(x, y, x + size.X, y + size.Y), awards);
            AwardAreas[index]->setHoverItem(getAwardPopup(index, -1));
            Achieved[index] = settings::gp_profileManager->getCurrentProfile()->hasMainAchievement(index);
        }
        for (int i = 0; i < settings::ACHIEVEMENT_MINI_COUNT; ++i)
        {
            for (int planet = 0; planet < 3; ++planet, ++index)
            {
                ox::core::CPosition2d<int> size(48, 48);
                if (MiniAwards[i * 3 + planet])
                    size = MiniAwards[i * 3 + planet]->getFrameSize(0);
                int y = AWARD_POS_Y[index] - size.Y / 2;
                int x = AWARD_POS_X[index] - size.X / 2;
                if (y > 340)
                    y -= 40;
                AwardAreas[index] = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(x, y, x + size.X, y + size.Y), awards);
                AwardAreas[index]->setHoverItem(getAwardPopup(i + settings::ACHIEVEMENT_MAIN_COUNT, planet));
                Achieved[index] = settings::gp_profileManager->getCurrentProfile()->hasMiniAchievement(i + settings::ACHIEVEMENT_MAIN_COUNT, planet);
            }
        }

        GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), Frame, ID_CLOSE, L"Ok")->LayoutFlags = "center br";
        Frame->sortRiver(true, 5, 5, false);
        Frame->centerOnParent();
        if (!Selector[0])
            Selector[0] = sprites->addNewAnimationState("ModeBtnSmallSelector");
    }
    else if (Window)
    {
        Window->remove();
        Window = 0;
    }
}

ox::gui::IGUIWindow* CAchievementsScreen::getAwardPopup(int achievement, int planet)
{
    ox::gui::IGUIWindow* popup = GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 90, 10),
        GUIEnvironment->getHoverParentElement(), -1);
    popup->setAnimations(Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", false), "Tooltip");
    ox::core::CString<wchar_t> title = settings::gp_systemConfig->getLocalizedText(settings::ACHIEVEMENT_NAMES[achievement]);
    ox::core::CString<wchar_t> description =
        settings::gp_systemConfig->getLocalizedText(settings::ACHIEVEMENT_DESCS[achievement]);
    if (planet >= 0)
    {
        title.append(ox::core::CString<wchar_t>(L" ("));
        title.append(ox::core::CString<wchar_t>(game::PlanetNames[planet]));
        title.append(ox::core::CString<wchar_t>(L")"));
    }
    ox::gui::IGUIStaticText* text = GUIEnvironment->addStaticText(title.c_str(), 600, popup,
        GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/largeFont.fnt"), -1, L"");
    text->setOverrideColor(ox::video::SColor(0xffffffff));
    text->LayoutFlags = "br center";
    text->packSize();
    text = GUIEnvironment->addStaticText(description.c_str(), 400, popup, 0, -1, L"");
    text->setOverrideColor(ox::video::SColor(0xffffffff));
    text->LayoutFlags = "br center";
    text->packSize();
    if (settings::CHarvestProfile::isMultiPlanetAchievement(achievement))
    {
        ox::gui::IGUIImage* icon = GUIEnvironment->addImage(ox::core::CRect<int>(0, 0, 30, 30), popup, -1, 0);
        icon->setAnimation("PlanetIcon1",
            Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true));
        icon->LayoutFlags = "br";
        if (!settings::gp_profileManager->getCurrentProfile()->hasMainAchievementAtPlanet(achievement, 0))
            icon->setOverrideColor(ox::video::SColor(0x80808080));
        icon = GUIEnvironment->addImage(ox::core::CRect<int>(0, 0, 30, 30), popup, -1, 0);
        icon->setAnimation("PlanetIcon2",
            Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true));
        if (!settings::gp_profileManager->getCurrentProfile()->hasMainAchievementAtPlanet(achievement, 1))
            icon->setOverrideColor(ox::video::SColor(0x80808080));
        icon = GUIEnvironment->addImage(ox::core::CRect<int>(0, 0, 30, 30), popup, -1, 0);
        icon->setAnimation("PlanetIcon3",
            Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true));
        if (!settings::gp_profileManager->getCurrentProfile()->hasMainAchievementAtPlanet(achievement, 2))
            icon->setOverrideColor(ox::video::SColor(0x80808080));
    }
    popup->sortRiver(true, 5, 5, false);
    return popup;
}

bool CAchievementsScreen::isVisible()
{
    if (Window)
        return Window->isVisible();
    return false;
}

} // end namespace gui
} // end namespace harvest
