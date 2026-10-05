// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <math.h>
#include <time.h>
#include <string.h>
#include <iostream>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CHarvestEntity.h"
#include "CPlayState.h"
#include "CLoadingScreen.h"
#include "harvest/CHarvestFullMain.h"
#include "harvest/ECustomEvents.h"
#include "harvest/entity/CAlienEntity.h"
#include "harvest/entity/CBuildableItems.h"
#include "harvest/entity/CConstructionEntity.h"
#include "harvest/entity/CCreativeEntity.h"
#include "harvest/entity/CDefenseTowerEntity.h"
#include "harvest/entity/CDropshipEntity.h"
#include "harvest/entity/CEntityManager.h"
#include "harvest/entity/CMinerEntity.h"
#include "harvest/entity/CSparkMoverEntity.h"
#include "harvest/entity/CSparkProducerEntity.h"
#include "harvest/game/CLuaManager.h"
#include "harvest/game/CScenario.h"
#include "harvest/game/CStatistics.h"
#include "harvest/game/CThreatLevel.h"
#include "harvest/game/SInfoLineMessage.h"
#include "harvest/gui/CGuiEffects.h"
#include "harvest/gui/CAchievementsScreen.h"
#include "harvest/gui/CGuiInfoLines.h"
#include "harvest/gui/CIngameMenuScreen.h"
#include "harvest/gui/CPriorityScreen.h"
#include "harvest/gui/CSaveGameScreen.h"
#include "harvest/gui/CSettingsScreen.h"
#include "harvest/gui/CStoryScreen.h"
#include "harvest/settings/CSystemConfig.h"
#include "harvest/settings/CHarvestProfile.h"
#include "harvest/settings/CProfileManager.h"
#include "harvest/settings/CSavestateInfo.h"
#include "harvest/settings/CAlienPriorities.h"
#include "ox/IOSOperator.h"
#include "ox/IOxDevice.h"
#include "ox/ITimer.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/algo/CRand.h"
#include "ox/core/CAes.h"
#include "ox/core/CCipherKey.h"
#include "ox/core/CBasic.h"
#include "ox/core/CHiddenInt.h"
#include "ox/core/CMath.h"
#include "ox/core/CStringFunctions.h"
#include "ox/gui/ICursorControl.h"
#include "ox/gui/IGUICheckBox.h"
#include "ox/gui/IGUIEditBox.h"
#include "ox/gui/IGUIElement.h"
#include "ox/gui/IGUIButton.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/gui/IGUILayout.h"
#include "ox/gui/IGUISkin.h"
#include "ox/gui/IGUIStaticText.h"
#include "ox/gui/IGUIWindow.h"
#include "ox/io/CHelpIO.h"
#include "ox/io/CMemReadFile.h"
#include "ox/io/CMemWriteFile.h"
#include "ox/io/IFileSystem.h"
#include "ox/io/IReadFile.h"
#include "ox/io/IWriteFile.h"
#include "ox/video/IParticleState.h"
#include "ox/video/IParticlePackage.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/ITexture.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {
namespace game {

//! Replaces the credit counters with new empty ones.
static inline void createCredits()
{
    delete gp_mineralAmount;
    gp_mineralAmount = new ox::core::CHiddenInt();
    delete gp_negatedMineralAmount;
    gp_negatedMineralAmount = new ox::core::CHiddenInt();
}

//! Sets the credits and their negated copy, which catches memory editing.
inline void setCredits(int credits)
{
    gp_mineralAmount->setValue(credits);
    gp_negatedMineralAmount->setValue(-credits);
}

} // end namespace game

namespace states {

const ox::video::SColor RANGE_CIRCLE_COLOR(0x80ffffa8);
const ox::video::SColor RANGE_LINE_COLOR(0xc8ffffa8);
const ox::video::SColor INVALID_CIRCLE_COLOR(0x80ff4040);
const ox::video::SColor MINING_CIRCLE_COLOR(0x8080ff80);
const ox::video::SColor MINING_LINE_COLOR(0xc880ff80);
const ox::video::SColor LASER_CIRCLE_COLOR(0xa0cc4040);
const ox::video::SColor WHITE_TEXT_COLOR(0xffe6f2f2);
const ox::video::SColor MINERALS_TEXT_COLOR(0xffb4c9ff);
const ox::video::SColor ENERGY_TEXT_COLOR(0xfffed555);
const ox::video::SColor THREAT_TEXT_COLOR(0xfffe8280);
const ox::video::SColor MINIMAP_MARKER_COLOR(0xa0dcdcdc);

//! The localization keys of the game speeds.
const wchar_t* const GAME_SPEED_NAMES[] = {L"gamespeed:pause", L"gamespeed:half", L"gamespeed:threequarter",
    L"gamespeed:normal", L"gamespeed:threehalfs", L"gamespeed:double", L"gamespeed:quadruple"};
//! How fast the game runs at each game speed.
const float GAME_SPEED_MULTIPLIERS[] = {0.0f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 4.0f};
//! The sprites of the alien types, loaded ahead for the aliens of the planet.
const char* const ALIEN_SPRITE_NAMES[] = {"Alien", "AlienJammer", "AlienTiny0000", "AlienSummoner", "AlienLooker0000",
    "AlienHogger", "AlienSparker", "BrainBottom0000", "MegaAlienCharge", "aliennames:asdf", "aliennames:asdf",
    "aliennames:asdf", "aliennames:asdf", "aliennames:asdf"};

//! Swaps the corners where the rectangle is upside down, as Irrlicht's rect::repair.
static inline void repairRect(ox::core::CRect<float>& rect)
{
    if (rect.LowerRightCorner.X < rect.UpperLeftCorner.X)
    {
        float t = rect.LowerRightCorner.X;
        rect.LowerRightCorner.X = rect.UpperLeftCorner.X;
        rect.UpperLeftCorner.X = t;
    }

    if (rect.LowerRightCorner.Y < rect.UpperLeftCorner.Y)
    {
        float t = rect.LowerRightCorner.Y;
        rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y;
        rect.UpperLeftCorner.Y = t;
    }
}

bool CPlayState::m_keys[256];

bool displayThreatLevelForGameMode(int gameMode)
{
    return gameMode == game::EGM_NORMAL || gameMode == game::EGM_INSANE || gameMode == game::EGM_CAMPAIGN ||
        gameMode == game::EGM_CREATIVE;
}

bool displayTimerForGameMode(int gameMode)
{
    if (gameMode == game::EGM_CREATIVE && game::gp_luaManager)
        return game::gp_luaManager->isTimerVisible();
    return gameMode == game::EGM_WAVE || gameMode == game::EGM_RUSH;
}

CPlayState::CPlayState()
    : MiddleMouseScrolling(false), LoadingScreen(0), InitStep(0), BoldFont(0), SmallFont(0), NumberFont(0),
      IngamePackage(0), MenuPackage(0), MousePosition(1, 1), Action(0), SelectedEntity(0), RecycleTarget(0),
      FollowJump(false), HasLastPlacement(false), DraggingFromSelection(false), SelectionClickPending(false),
      RectangleSelecting(false), PlacementOk(false), BuildSelection(0), RangeCircle(0), Selector(0), RecycleSelector(0),
      Beam180(6.0f), Beam1c8(2.0f), Beam210(6.0f), Beam258(6.0f), m_2a0(false), ThreatLevel(0), Scenario(0),
      GameSpeed(3), m_2c0(0), GameOver(false), GameWon(false), GameOverTime(0), MinimapDragging(false), MinimapDot(0),
      MinimapTexture(0), UseMinimapTexture(false), MinimapUpdateTime(0), StatsTime(0), HarvestingCount(0),
      OverheatedCount(0), GameTime(0), Victory(false), LevelRecordShown(false), MineralsRecordShown(false),
      RecordCheckTime(0), DenialTime(0), CreditsText(0), HarvestersText(0), ThreatLevelText(0), BottomBar(0),
      ActionPanel(0), TopBar(0), MinimapWidth(0), MinimapHeight(0), RecycleButton(0), InfoText(0),
      BuildingsScrolling(false), HighlightActive(false), HighlightTime(0), HighlightEntityType(0), HighlightLayer(0),
      ShowAllRanges(false), m_7b0(0), SettingsScreen(0), PriorityScreen(0), IngameMenuScreen(0), SaveGameScreen(0),
      StoryScreen(0), AchievementsScreen(0), InfoLines(0), Profile(0), ParticleSetting(2), ScrollSpeed(1.0f),
      ShowDebugInfo(false), ShowMouseWorldPos(false), UpdateDuration(0), RenderDuration(0), BuyMessageBox(0)
{
    for (int i = 0; i < 256; ++i)
        m_keys[i] = false;
    for (int i = 0; i < GS_COUNT; ++i)
        GuiSprites[i] = 0;
    for (int i = 0; i < WAVE_COUNT; ++i)
    {
        WaveIcons[i] = 0;
        m_488[i] = 0;
    }
    for (int i = 0; i < LIST_COUNT; ++i)
    {
        ListVisible[i] = false;
        ListGroups[i] = 0;
        ListContents[i] = 0;
        ListButtons[i] = 0;
    }
    LuaManager = 0;
}

CPlayState::~CPlayState()
{
    if (AudioDriver)
        AudioDriver->stopAllMusic();
    if (LoadingScreen)
        delete LoadingScreen;

    delete game::gp_world;
    game::gp_world = 0;
    delete game::gp_mineralAmount;
    game::gp_mineralAmount = 0;
    delete game::gp_negatedMineralAmount;
    game::gp_negatedMineralAmount = 0;
    delete ThreatLevel;
    delete entity::gp_entityManager;
    entity::gp_entityManager = 0;
    delete Scenario;

    delete SettingsScreen;
    delete PriorityScreen;
    delete IngameMenuScreen;
    delete SaveGameScreen;
    delete StoryScreen;
    delete InfoLines;
    delete AchievementsScreen;

    if (MinimapTexture)
    {
        MinimapTexture->drop();
        MinimapTexture = 0;
    }
    if (MinimapDot)
        MinimapDot->remove();
    if (RangeCircle)
        RangeCircle->remove();
    if (Selector)
        Selector->remove();
    if (RecycleSelector)
        RecycleSelector->remove();
    if (BottomBar)
        BottomBar->remove();
    if (ActionPanel)
        ActionPanel->remove();
    if (TopBar)
        TopBar->remove();
    if (m_7b0)
        m_7b0->remove();
    for (int i = 0; i < LIST_COUNT; ++i)
        if (ListGroups[i])
            ListGroups[i]->remove();
    for (int i = 0; i < GS_COUNT; ++i)
        if (GuiSprites[i])
            GuiSprites[i]->remove();
    for (int i = 0; i < WAVE_COUNT; ++i)
    {
        if (WaveIcons[i])
            WaveIcons[i]->remove();
        if (m_488[i])
            m_488[i]->remove();
    }
    delete LuaManager;
}

int CPlayState::firstInit(ox::IOxDevice* device)
{
    if (CGameState::firstInit(device) != 0)
        return 1;

    LoadingScreen = new CLoadingScreen();
    if (LoadingScreen->init(Driver) == 1)
        return 1;

    Device->setEventReceiver(this);
    return 0;
}

int CPlayState::secondInit()
{
    switch (InitStep++)
    {
    case 0:
        BoldFont = GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt");
        SmallFont = GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/smallFont.fnt");
        NumberFont = GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/mineralNumbers.fnt");
        GUIEnvironment->getSkin()->setFont(BoldFont);
        if (!BoldFont || !SmallFont)
            return 1;
        BoldFontHeight = BoldFont->getDimension(L"A").Height;
        SmallFontHeight = SmallFont->getDimension(L"A").Height;
        break;
    case 1:
        IngamePackage = Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", false);
        if (!IngamePackage)
            return 1;
        MenuPackage = Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true);
        break;
    case 2:
        GUIEnvironment->getSkin()->setSpritePackage(
            Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true));
        GUIEnvironment->getSkin()->setColor((ox::gui::EGUI_DEFAULT_COLOR)8, ox::video::SColor(0xffffffff));
        break;
    case 3:
        ScreenSize = Driver->getScreenSize();
        ScreenSizeF = ox::core::CDimension2d<float>((float)ScreenSize.Width, (float)ScreenSize.Height);
        break;
    case 4:
        entity::CEntity::gp_spritePackage = IngamePackage;
        entity::CEntity::gp_particlePackage =
            Driver->getParticlePackage("$GAME_RESOURCES$/harvestClientData/gfx/particles.pfx");
        if (!entity::CEntity::gp_particlePackage)
            return 1;
        entity::CEntity::gp_particlePackage->setSpritePackage(
            Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/fx.dat", false));
        entity::CEntity::gp_particlePackage->setCallbackEngine(this);
        entity::CEntity::gp_particlePackage->ImportanceLevel = 2;
        entity::CEntity::gp_videoDriver = Driver;
        entity::CEntity::gp_audioDriver = AudioDriver;
        entity::CEntity::gp_alienChantFont = SmallFont;
        break;
    case 6:
        BuildableItems.loadStandardBuildings(Driver);
        break;
    case 7:
    {
        // Loads the background animation ahead.
        ox::video::ISpriteAnimationState* background = IngamePackage->addNewAnimationState("Background");
        if (background)
            background->remove();
        break;
    }
    case 8:
        MinimapDot = IngamePackage->addNewAnimationState("MinimapDot");
        MinimapRect = ox::core::CRect<int>(ScreenSize.Width - 110, 10, ScreenSize.Width - 10, 110);
        break;
    case 9:
        RangeCircle = IngamePackage->addNewAnimationState("RangeCircle");
        Beam180.Beam = IngamePackage->addNewAnimationState("RangeLine");
        Beam1c8.Beam = IngamePackage->addNewAnimationState("EnergyRedirect");
        Beam210.Beam = IngamePackage->addNewAnimationState("RangeLine");
        Beam258.Beam = IngamePackage->addNewAnimationState("RangeLine");
        Beam180.Color = RANGE_LINE_COLOR;
        Beam210.Color = MINING_LINE_COLOR;
        Beam1c8.Color = ox::video::SColor(0x80ffffff);
        break;
    case 10:
        GuiSprites[GS_TOP_RIGHT_BACKGROUND] = IngamePackage->addNewAnimationState("GuiTopRightBackground");
        GuiSprites[GS_THREAT_LEVEL_BACKGROUND] = IngamePackage->addNewAnimationState("GuiThreatLevelBackground");
        GuiSprites[GS_TIME_BACKGROUND] = IngamePackage->addNewAnimationState("GuiTimeBackground");
        GuiSprites[GS_PROGRESS_BAR] = IngamePackage->addNewAnimationState("GuiProgressBar");
        GuiSprites[GS_DAMAGE_BAR_BACKGROUND] = IngamePackage->addNewAnimationState("GuiDamageBarBackground");
        GuiSprites[GS_DAMAGE_BAR] = IngamePackage->addNewAnimationState("GuiDamageBar");
        GuiSprites[GS_OBJECTIVES_BACKGROUND] = IngamePackage->addNewAnimationState("GuiObjectivesBackground");
        GuiSprites[GS_TOP_LEFT_BACKGROUND] = IngamePackage->addNewAnimationState("GuiTopLeftBackground");
        GuiSprites[GS_BOTTOM_LEFT_BACKGROUND] = IngamePackage->addNewAnimationState("GuiBottomLeftBackground");
        GuiSprites[GS_BOTTOM_RIGHT_BACKGROUND] = IngamePackage->addNewAnimationState("GuiBottomRightBackground");
        GuiSprites[GS_BOTTOM_CENTER_BACKGROUND] = IngamePackage->addNewAnimationState("GuiBottomCenterBackground");
        GuiSprites[GS_ICON_ENERGY] = IngamePackage->addNewAnimationState("IconEnergy");
        GuiSprites[GS_ICON_CREDITS] = IngamePackage->addNewAnimationState("IconCredits");
        GuiSprites[GS_ICON_OBJECTIVE] = IngamePackage->addNewAnimationState("IconObjective");
        break;
    case 11:
        GuiSprites[GS_MINIMAP_TOP_LEFT] = IngamePackage->addNewAnimationState("MinimapTopLeft");
        GuiSprites[GS_MINIMAP_TOP] = IngamePackage->addNewAnimationState("MinimapTop");
        GuiSprites[GS_MINIMAP_TOP_RIGHT] = IngamePackage->addNewAnimationState("MinimapTopRight");
        GuiSprites[GS_MINIMAP_LEFT] = IngamePackage->addNewAnimationState("MinimapLeft");
        GuiSprites[GS_MINIMAP_BOTTOM_LEFT] = IngamePackage->addNewAnimationState("MinimapBottomLeft");
        GuiSprites[GS_MINIMAP_RIGHT] = IngamePackage->addNewAnimationState("MinimapRight");
        GuiSprites[GS_MINIMAP_BOTTOM] = IngamePackage->addNewAnimationState("MinimapBottom");
        GuiSprites[GS_MINIMAP_BOTTOM_RIGHT] = IngamePackage->addNewAnimationState("MinimapBottomRight");
        GuiSprites[GS_MINIMAP_BACKGROUND] = IngamePackage->addNewAnimationState("MinimapBackground");
        GuiSprites[GS_MINERALS_BACKGROUND] = IngamePackage->addNewAnimationState("GuiMineralsBackground");
        break;
    case 12:
    {
        BottomBar = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 400, 50), 0);
        BottomBar->setID(GUI_ID_BOTTOM_BAR);
        BottomBar->setReportOnDraw(1);

        GuiElements[GUI_ID_PRIORITIES] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), BottomBar, GUI_ID_PRIORITIES, 0);
        GuiElements[GUI_ID_PRIORITIES]->setAnimations(IngamePackage, "BtnPriorities", true);
        GuiElements[GUI_ID_PRIORITIES]->setHoverItem(
            getPopupForGuiButton(settings::gp_systemConfig->getLocalizedText(L"gamepopups:prios").c_str()));
        GuiElements[GUI_ID_SPEED_PAUSE] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), BottomBar, GUI_ID_SPEED_PAUSE, 0);
        GuiElements[GUI_ID_SPEED_PAUSE]->setAnimations(IngamePackage, "BtnSpeed1", true);
        GuiElements[GUI_ID_SPEED_PAUSE]->setHoverItem(
            getPopupForGuiButton(settings::gp_systemConfig->getLocalizedText(L"gamepopups:pause").c_str()));
        GuiElements[GUI_ID_SPEED_SLOW] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), BottomBar, GUI_ID_SPEED_SLOW, 0);
        GuiElements[GUI_ID_SPEED_SLOW]->setAnimations(IngamePackage, "BtnSpeed2", true);
        GuiElements[GUI_ID_SPEED_SLOW]->setHoverItem(
            getPopupForGuiButton(settings::gp_systemConfig->getLocalizedText(L"gamepopups:slow").c_str()));
        GuiElements[GUI_ID_SPEED_NORMAL] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), BottomBar, GUI_ID_SPEED_NORMAL, 0);
        GuiElements[GUI_ID_SPEED_NORMAL]->setAnimations(IngamePackage, "BtnSpeed3", true);
        GuiElements[GUI_ID_SPEED_NORMAL]->setHoverItem(
            getPopupForGuiButton(settings::gp_systemConfig->getLocalizedText(L"gamepopups:normal").c_str()));
        GuiElements[GUI_ID_SPEED_DOUBLE] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), BottomBar, GUI_ID_SPEED_DOUBLE, 0);
        GuiElements[GUI_ID_SPEED_DOUBLE]->setAnimations(IngamePackage, "BtnSpeed4", true);
        GuiElements[GUI_ID_SPEED_DOUBLE]->setHoverItem(
            getPopupForGuiButton(settings::gp_systemConfig->getLocalizedText(L"gamepopups:double").c_str()));
        GuiElements[GUI_ID_SPEED_FOUR] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), BottomBar, GUI_ID_SPEED_FOUR, 0);
        GuiElements[GUI_ID_SPEED_FOUR]->setAnimations(IngamePackage, "BtnSpeed5", true);
        GuiElements[GUI_ID_SPEED_FOUR]->setHoverItem(
            getPopupForGuiButton(settings::gp_systemConfig->getLocalizedText(L"gamepopups:four").c_str()));
        GuiElements[GUI_ID_BUILDINGS_LEFT] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), BottomBar, GUI_ID_BUILDINGS_LEFT, 0);
        GuiElements[GUI_ID_BUILDINGS_LEFT]->setAnimations(IngamePackage, "BtnBuildingsLeft", true);
        GuiElements[GUI_ID_BUILDINGS_RIGHT] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), BottomBar, GUI_ID_BUILDINGS_RIGHT, 0);
        GuiElements[GUI_ID_BUILDINGS_RIGHT]->setAnimations(IngamePackage, "BtnBuildingsRight", true);
        GuiElements[GUI_ID_MENU] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), BottomBar, GUI_ID_MENU, 0);
        // The menu button's animation is localized.
        GuiElements[GUI_ID_MENU]->setAnimations(IngamePackage,
            ox::core::CString<char>(settings::gp_systemConfig->getLocalizedText(L"game:menuBtn").c_str()).c_str(),
            true);

        RecycleButton = GUIEnvironment->addCheckBox(false, ox::core::CRect<int>(0, 0, 40, 40), BottomBar,
            GUI_ID_RECYCLE, 0);
        RecycleButton->setAnimations(IngamePackage, "Recycle");
        RecycleButton->setHoverItem(
            getPopupForGuiButton(settings::gp_systemConfig->getLocalizedText(L"gamepopups:recycle").c_str()));

        BuildingsArea = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 100, 50), BottomBar);
        BuildingsList = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 100, 50), BuildingsArea);
        BuildingsList->setID(GUI_ID_BUILDINGS_LIST);
        BuildingsList->setReportOnDraw(2);

        ActionPanel = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 400, 50), 0);
        ActionPanel->setID(GUI_ID_ACTION_PANEL);
        ActionPanel->setReportOnDraw(1);

        InfoText = GUIEnvironment->addStaticText(L"", ox::core::CRect<int>(0, 0, 90, 20), false, false, ActionPanel, -1, 0);
        InfoText->setOverrideColor(WHITE_TEXT_COLOR);
        InfoText->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);
        InfoText->setOverrideFont(BoldFont);
        SelectedNameText = GUIEnvironment->addStaticText(L"", ox::core::CRect<int>(0, 0, 90, 20), false, false, ActionPanel, -1, 0);
        SelectedNameText->setOverrideColor(WHITE_TEXT_COLOR);
        SelectedNameText->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);
        SelectedNameText->setOverrideFont(BoldFont);
        OperatorText = GUIEnvironment->addStaticText(L"", ox::core::CRect<int>(0, 0, 90, 20), false, false, ActionPanel, -1, 0);
        OperatorText->setOverrideColor(ox::video::SColor(0xffb1c7ff));
        OperatorText->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);
        OperatorText->setOverrideFont(SmallFont);
        MiniStatText = GUIEnvironment->addStaticText(L"", ox::core::CRect<int>(0, 0, 90, 20), false, false, ActionPanel, -1, 0);
        MiniStatText->setOverrideColor(ox::video::SColor(0xffb1c7ff));
        MiniStatText->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);
        MiniStatText->setOverrideFont(SmallFont);

        GuiElements[GUI_ID_DESELECT] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), ActionPanel, GUI_ID_DESELECT, 0);
        GuiElements[GUI_ID_DESELECT]->setAnimations(IngamePackage, "BtnActionDeselect", true);
        GuiElements[GUI_ID_DESELECT]->setVisible(false);
        GuiElements[GUI_ID_DESELECT]->setHoverItem(getPopupForGuiButton(
            settings::gp_systemConfig->getLocalizedText(L"gamepopups:deselect", L"ESC").c_str()));
        GuiElements[GUI_ID_UNLINK] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), ActionPanel, GUI_ID_UNLINK, 0);
        GuiElements[GUI_ID_UNLINK]->setAnimations(IngamePackage, "BtnActionUnlink", true);
        GuiElements[GUI_ID_UNLINK]->setVisible(false);
        GuiElements[GUI_ID_UNLINK]->setHoverItem(
            getPopupForGuiButton(settings::gp_systemConfig->getLocalizedText(L"gamepopups:unlink").c_str()));
        GuiElements[GUI_ID_SPEED_BUILD] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), ActionPanel, GUI_ID_SPEED_BUILD, 0);
        GuiElements[GUI_ID_SPEED_BUILD]->setAnimations(IngamePackage, "BtnActionSpeedBuild", true);
        GuiElements[GUI_ID_SPEED_BUILD]->setVisible(false);
        GuiElements[GUI_ID_SPEED_BUILD]->setHoverItem(getPopupForGuiButton(
            settings::gp_systemConfig->getLocalizedText(L"gamepopups:speedBuild", L"Z").c_str()));
        GuiElements[GUI_ID_UNLINK_SPEED_BUILD] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), ActionPanel, GUI_ID_UNLINK_SPEED_BUILD,
            0);
        GuiElements[GUI_ID_UNLINK_SPEED_BUILD]->setAnimations(IngamePackage, "BtnActionUnlinkSpeedBuild", true);
        GuiElements[GUI_ID_UNLINK_SPEED_BUILD]->setVisible(false);
        GuiElements[GUI_ID_UNLINK_SPEED_BUILD]->setHoverItem(getPopupForGuiButton(
            settings::gp_systemConfig->getLocalizedText(L"gamepopups:unlinkSpeedBuild", L"Z").c_str()));
        GuiElements[GUI_ID_OVERCHARGE] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), ActionPanel, GUI_ID_OVERCHARGE, 0);
        GuiElements[GUI_ID_OVERCHARGE]->setAnimations(IngamePackage, "BtnActionOvercharge", true);
        GuiElements[GUI_ID_OVERCHARGE]->setVisible(false);
        GuiElements[GUI_ID_OVERCHARGE]->setHoverItem(getPopupForBuildButton(
            settings::gp_systemConfig->getLocalizedText(L"build:bombBuilding").c_str(),
            settings::gp_systemConfig->getLocalizedText(L"buildpopups:bombBuilding", L"X").c_str(), 30, 0));
        GuiElements[GUI_ID_REPLACE_PRODUCER] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), ActionPanel, GUI_ID_REPLACE_PRODUCER, 0);
        GuiElements[GUI_ID_REPLACE_PRODUCER]->setAnimations(IngamePackage, "BtnActionReplaceProducer", true);
        GuiElements[GUI_ID_REPLACE_PRODUCER]->setVisible(false);
        GuiElements[GUI_ID_REPLACE_PRODUCER]->setHoverItem(getPopupForBuildButton(
            settings::gp_systemConfig->getLocalizedText(L"build:replaceProducer").c_str(),
            settings::gp_systemConfig->getLocalizedText(L"buildpopups:replaceProducer", L"X").c_str(), 15, 25));
        GuiElements[GUI_ID_SELL_HARVESTERS] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), ActionPanel, GUI_ID_SELL_HARVESTERS, 0);
        GuiElements[GUI_ID_SELL_HARVESTERS]->setAnimations(IngamePackage, "BtnActionSellHarvesters", true);
        GuiElements[GUI_ID_SELL_HARVESTERS]->setVisible(false);
        GuiElements[GUI_ID_SELL_HARVESTERS]->setHoverItem(getPopupForGuiButton(
            settings::gp_systemConfig->getLocalizedText(L"gamepopups:sellHarvesters", L"X").c_str()));
        GuiElements[GUI_ID_EAGLE] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), ActionPanel, GUI_ID_EAGLE, 0);
        GuiElements[GUI_ID_EAGLE]->setAnimations(IngamePackage, "BtnActionEagle", true);
        GuiElements[GUI_ID_EAGLE]->setVisible(false);
        GuiElements[GUI_ID_EAGLE]->setHoverItem(getPopupForBuildButton(
            settings::gp_systemConfig->getLocalizedText(L"build:eagle").c_str(),
            settings::gp_systemConfig->getLocalizedText(L"buildpopups:eagle", L"C").c_str(), 100, 50));
        GuiElements[GUI_ID_TEMPEST] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), ActionPanel, GUI_ID_TEMPEST, 0);
        GuiElements[GUI_ID_TEMPEST]->setAnimations(IngamePackage, "BtnActionTempest", true);
        GuiElements[GUI_ID_TEMPEST]->setVisible(false);
        GuiElements[GUI_ID_TEMPEST]->setHoverItem(getPopupForBuildButton(
            settings::gp_systemConfig->getLocalizedText(L"build:tempest").c_str(),
            settings::gp_systemConfig->getLocalizedText(L"buildpopups:tempest", L"V").c_str(), 100, 40));
        GuiElements[GUI_ID_DEATHSTAR] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), ActionPanel, GUI_ID_DEATHSTAR, 0);
        GuiElements[GUI_ID_DEATHSTAR]->setAnimations(IngamePackage, "BtnActionDeathStar", true);
        GuiElements[GUI_ID_DEATHSTAR]->setVisible(false);
        GuiElements[GUI_ID_DEATHSTAR]->setHoverItem(getPopupForGuiButton(
            settings::gp_systemConfig->getLocalizedText(L"gamepopups:deathstar", L"X").c_str()));
        GuiElements[GUI_ID_UNLINK_DEATHSTAR] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), ActionPanel, GUI_ID_UNLINK_DEATHSTAR, 0);
        GuiElements[GUI_ID_UNLINK_DEATHSTAR]->setAnimations(IngamePackage, "BtnActionUnlinkDeathStar", true);
        GuiElements[GUI_ID_UNLINK_DEATHSTAR]->setVisible(false);
        GuiElements[GUI_ID_UNLINK_DEATHSTAR]->setHoverItem(getPopupForGuiButton(
            settings::gp_systemConfig->getLocalizedText(L"gamepopups:unlinkDeathstar", L"X").c_str()));
        GuiElements[GUI_ID_END_LASER] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), ActionPanel, GUI_ID_END_LASER, 0);
        GuiElements[GUI_ID_END_LASER]->setAnimations(IngamePackage, "BtnActionEndLaser", true);
        GuiElements[GUI_ID_END_LASER]->setVisible(false);
        GuiElements[GUI_ID_END_LASER]->setHoverItem(getPopupForGuiButton(
            settings::gp_systemConfig->getLocalizedText(L"gamepopups:makeEndLaser", L"Z").c_str()));
        // The lua action buttons are created by the scenario.
        for (int i = GUI_ID_FIRST_LUA_ACTION; i < GUI_ID_WAVE_SEND; ++i)
            GuiElements[i] = 0;

        TopBar = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 400, 50), 0);
        TopBar->setID(GUI_ID_TOP_BAR);
        TopBar->setReportOnDraw(1);

        CreditsText = GUIEnvironment->addStaticText(L"75",
            ox::core::CRect<int>(ScreenSize.Width - 260, 5, ScreenSize.Width - 183, 25), false, false, TopBar, -1, 0);
        CreditsText->setOverrideColor(MINERALS_TEXT_COLOR);
        CreditsText->setOverrideFont(NumberFont);
        CreditsText->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);
        CreditsText->setHoverItem(
            getPopupForGuiButton(settings::gp_systemConfig->getLocalizedText(L"gamepopups:credits").c_str()));
        HarvestersText = GUIEnvironment->addStaticText(L"+0",
            ox::core::CRect<int>(ScreenSize.Width - 183, 5, ScreenSize.Width - 153, 25), false, false, TopBar, -1, 0);
        HarvestersText->setOverrideColor(MINERALS_TEXT_COLOR);
        HarvestersText->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);
        HarvestersText->setOverrideFont(SmallFont);
        HarvestersText->setHoverItem(
            getPopupForGuiButton(settings::gp_systemConfig->getLocalizedText(L"gamepopups:harvesters").c_str()));
        ThreatLevelText = GUIEnvironment->addStaticText(L"0",
            ox::core::CRect<int>(ScreenSize.Width - 180, 5, ScreenSize.Width - 130, 25), false, false, TopBar, -1, 0);
        ThreatLevelText->setOverrideColor(THREAT_TEXT_COLOR);
        ThreatLevelText->setOverrideFont(NumberFont);
        ThreatLevelText->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);

        m_7b0 = GUIEnvironment->addEditBox(L"", ox::core::CRect<int>(0, 0, 100, 10), false, 0, GUI_ID_SCRIPT_INPUT);
        m_7b0->setVisible(false);
        m_7b0->setEnabled(false);
        break;
    }
    case 13:
        SettingsScreen = new gui::CSettingsScreen(Device, false);
        PriorityScreen = new gui::CPriorityScreen(Device);
        IngameMenuScreen = new gui::CIngameMenuScreen(Device);
        break;
    case 14:
        SaveGameScreen = new gui::CSaveGameScreen(Device);
        StoryScreen = new gui::CStoryScreen(Device);
        InfoLines = new gui::CGuiInfoLines(Device);
        AchievementsScreen = new gui::CAchievementsScreen(Device);
        break;
    case 15:
        if (g_loadGameFilename.size() > 0)
        {
            if (!readStateFromFile(g_loadGameFilename.c_str()))
            {
                // A save that does not load starts a new game instead.
                g_loadGameFilename = "";
                if (!initializeNewGame())
                    return 1;
            }
            g_loadGameFilename = "";
        }
        else if (!initializeNewGame())
            return 1;
        break;
    case 16:
    {
        ox::video::ISpriteAnimationState* alien = IngamePackage->addNewAnimationState("Alien");
        if (alien)
            alien->remove();
        break;
    }
    case 17:
        Selector = IngamePackage->addNewAnimationState("Selector");
        RecycleSelector = IngamePackage->addNewAnimationState("RecycleSelector");
        break;
    case 19:
        if (GameMode == game::EGM_CREATIVE)
            ThreatLevelText->setHoverItem(
                getPopupForGuiButton(settings::gp_systemConfig->getLocalizedText(L"gamepopups:alienCount").c_str()));
        else
            ThreatLevelText->setHoverItem(
                getPopupForGuiButton(settings::gp_systemConfig->getLocalizedText(L"gamepopups:threat").c_str()));
        realignGui();
        break;
    case 36:
        NextState = 0;
        updateState(0.001f);
        break;
    case 20:
        for (int i = 0; i < WAVE_COUNT; ++i)
            if (ThreatLevel && game::gp_world)
                if (game::CThreatLevel::alienOccursOnPlanet(game::gp_world->getPlanet(), i))
                    m_488[i] = IngamePackage->addNewAnimationState(ALIEN_SPRITE_NAMES[i]);
        break;
    case 21:
        UseMinimapTexture = false;
        break;
    case 35:
        playPlanetMusic();
        break;
    // Idle steps, which give the loading screen time to show.
    case 5:
        return 2;
    case 18: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29: case 30: case 31:
    case 32: case 33: case 34:
        break;
    default:
        return 0;
    }
    return 2;
}

ox::gui::IGUILayout* CPlayState::getPopupForGuiButton(const wchar_t* text)
{
    ox::gui::IGUIWindow* popup = static_cast<ox::gui::IGUIWindow*>(
        GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 90, 10), GUIEnvironment->getHoverParentElement(), -1));
    popup->setAnimations(IngamePackage, "Tooltip");
    ox::gui::IGUIStaticText* label = GUIEnvironment->addStaticText(text, ox::core::CRect<int>(0, 0, 1000, 1000),
        false, true, popup, -1, 0);
    label->setOverrideColor(WHITE_TEXT_COLOR);
    label->setOverrideFont(BoldFont);
    label->packSize();
    popup->sortRiver(true, 5, 5, false);
    return popup;
}

ox::gui::IGUILayout* CPlayState::getPopupForBuildButton(const wchar_t* name, const wchar_t* description, int energy,
    int minerals)
{
    ox::gui::IGUILayout* popup =
        GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 100, 100), GUIEnvironment->getHoverParentElement());
    ox::gui::IGUIWindow* titleFrame = static_cast<ox::gui::IGUIWindow*>(GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 20, 20), popup, -1));
    titleFrame->setAnimations(IngamePackage, "Tooltip");
    ox::gui::IGUIWindow* mineralsFrame = static_cast<ox::gui::IGUIWindow*>(
        GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 20, 20), popup, GUI_ID_MINERALS_POPUP));
    mineralsFrame->setAnimations(IngamePackage, "Tooltip");
    mineralsFrame->setReportOnDraw(1);
    ox::gui::IGUIWindow* energyFrame = static_cast<ox::gui::IGUIWindow*>(
        GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 20, 20), popup, GUI_ID_ENERGY_POPUP));
    energyFrame->setAnimations(IngamePackage, "Tooltip");
    energyFrame->setReportOnDraw(1);
    ox::gui::IGUIWindow* descriptionFrame = static_cast<ox::gui::IGUIWindow*>(GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 240, 23), popup, -1));
    descriptionFrame->setAnimations(IngamePackage, "Tooltip");
    descriptionFrame->LayoutFlags = "br";

    ox::gui::IGUIStaticText* title = GUIEnvironment->addStaticText(name, 138, titleFrame, BoldFont, -1, L"");
    title->setOverrideColor(WHITE_TEXT_COLOR);
    title->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);
    titleFrame->sortRiver(true, 1, 3, false);
    title->move(ox::core::CPosition2d<int>(0, 1));

    ox::core::CString<wchar_t> text(L"   ");
    text.append(energy);
    ox::gui::IGUIStaticText* energyText = GUIEnvironment->addStaticText(text.c_str(), 50, energyFrame, BoldFont, -1, L"");
    energyText->setOverrideColor(ENERGY_TEXT_COLOR);
    energyText->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);
    energyFrame->sortRiver(true, 1, 3, false);
    energyText->move(ox::core::CPosition2d<int>(0, 1));

    text = L"   ";
    text.append(minerals);
    ox::gui::IGUIStaticText* mineralsText =
        GUIEnvironment->addStaticText(text.c_str(), 50, mineralsFrame, BoldFont, -1, L"");
    mineralsText->setOverrideColor(MINERALS_TEXT_COLOR);
    mineralsText->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);
    mineralsFrame->sortRiver(true, 1, 3, false);
    mineralsText->move(ox::core::CPosition2d<int>(0, 1));

    GUIEnvironment->addStaticText(description, 240, descriptionFrame, BoldFont, -1, L"")->setOverrideColor(
        WHITE_TEXT_COLOR);
    descriptionFrame->sortRiver(true, 5, 3, false);
    popup->sortRiver(true, 1, 3, false);
    return popup;
}

bool CPlayState::readStateFromFile(const char* filename)
{
    ox::io::IReadFile* file = Device->getFileSystem()->createAndOpenFile(filename);
    if (!file)
        return false;

    bool result = false;
    settings::SSavestateHeader header;
    if (!settings::CSavestateInfo::readHeader(file, header))
    {
        file->drop();
        return result;
    }

    if (settings::gp_profileManager && settings::gp_profileManager->getCurrentProfile())
        Profile = settings::gp_profileManager->getCurrentProfile();
    if (settings::gp_systemConfig)
    {
        ParticleSetting = settings::gp_systemConfig->getParticleSetting();
        ScrollSpeed = settings::gp_systemConfig->getScrollSpeed();
    }

    eraseGameObjects();
    game::createCredits();
    game::gp_statistics = new game::CStatistics();
    game::gp_world = new game::CWorld(GameMode, g_gamePlanet);
    entity::gp_entityManager = new entity::CEntityManager();
    entity::g_nextEntityId = 1;
    ThreatLevel = new game::CThreatLevel(g_gameMode);

    int compressedSize = ox::io::CHelpIO::readInt(file);
    int size = ox::io::CHelpIO::readInt(file);
    if (compressedSize <= 0)
    {
        file->drop();
        return result;
    }

    unsigned char* data;
    if (header.Version > 12)
    {
        int encryptedSize = ox::io::CHelpIO::readInt(file);
        ox::core::CAes aes;
        ox::algo::CRand random(1);
        char keyData[32];
        for (int i = 0; i < 32; ++i)
            keyData[i] = random.nextInt(256);
        ox::core::CCipherKey key(keyData, 32);
        aes.setKey(&key);

        unsigned char* encrypted = new unsigned char[encryptedSize];
        data = new unsigned char[encryptedSize];
        file->read(encrypted, encryptedSize);
        file->drop();
        aes.decrypt(data, encrypted, encryptedSize);
        delete[] encrypted;
    }
    else
    {
        // Older saves are scrambled with a rotating key.
        data = new unsigned char[compressedSize];
        file->read(data, compressedSize);
        file->drop();
        int i = 0;
        unsigned int scramble = 0x4f2c7b19;
        for (; i < compressedSize; ++i)
        {
            switch (i & 3)
            {
            case 0:
                data[i] ^= (scramble & 0xff000000) >> 24;
                break;
            case 1:
                data[i] ^= (scramble & 0xff0000) >> 16;
                break;
            case 2:
                data[i] ^= (scramble & 0xff00) >> 8;
                break;
            case 3:
            {
                data[i] ^= scramble & 0xff;
                // The key rotates right by one bit after every four bytes.
                unsigned int low = scramble & 1;
                scramble >>= 1;
                scramble |= low << 31;
                break;
            }
            }
        }
    }

    unsigned char* unpacked = new unsigned char[size];
    unsigned int written;
    if (!Device->getFileSystem()->zipInflateData(unpacked, size, data, compressedSize, written))
    {
        delete[] data;
        delete[] unpacked;
        return result;
    }

    ox::io::CMemReadFile* memFile = new ox::io::CMemReadFile(unpacked, size, false);
    StartTime = ox::io::CHelpIO::readInt(memFile);
    RandomValue = ox::io::CHelpIO::readInt(memFile);
    ox::io::CHelpIO::readWideString(memFile, PlayerName);
    ox::io::CHelpIO::readWideString(memFile, PlayerGroup);
    ViewPosition.X = ox::io::CHelpIO::readFloat(memFile);
    ViewPosition.Y = ox::io::CHelpIO::readFloat(memFile);
    GameMode = ox::io::CHelpIO::readInt(memFile);
    game::gp_world->GameMode = GameMode;
    ThreatLevel->read(memFile, header.Version);
    int nextEntityId = ox::io::CHelpIO::readInt(memFile);
    if (header.Version > 14)
    {
        game::gp_mineralAmount->read(memFile);
        game::setCredits(game::gp_mineralAmount->getValue());
    }
    else
        game::setCredits(ox::io::CHelpIO::readInt(memFile));

    if (header.Version > 18)
    {
        GameTime = ox::io::CHelpIO::readFloat(memFile);
        Victory = ox::io::CHelpIO::readInt(memFile) != 0;
    }
    else
        Victory = false;
    if (header.Version > 19)
        BuildingAttacked = ox::io::CHelpIO::readInt(memFile) != 0;
    else
        BuildingAttacked = false;
    LevelRecordShown = false;
    MineralsRecordShown = false;

    if (header.Version > 28)
    {
        if (ox::io::CHelpIO::readByte(memFile))
        {
            LuaManager = new game::CLuaManager(Device, this);
            if (!LuaManager->initLuaBySaveFile(memFile, header.Version))
            {
                delete LuaManager;
                LuaManager = 0;
            }
        }
    }

    if (!game::gp_world->readAndInitialize(memFile, header.Version, Driver, getViewSize()))
    {
        delete memFile;
        delete[] data;
        delete[] unpacked;
        return result;
    }

    game::gp_statistics->read(memFile, header.Version);
    if (header.Version > 10)
        for (int i = 0; i < 5; ++i)
            settings::g_attackPriorities[i].read(memFile, header.Version);
    if (GameMode == game::EGM_CREATIVE)
        BuildableItems.loadCreativeBuildings(Device);
    entity::gp_entityManager->readEntities(memFile, header.Version);
    entity::g_nextEntityId = nextEntityId;
    delete memFile;
    delete[] data;
    delete[] unpacked;

    // Let the entities settle once before the first frame.
    entity::gp_entityManager->update(0.001f,
        ox::core::CRect<float>(ViewPosition + ox::core::CPosition2d<float>(-100.0f, -100.0f),
            ox::core::CDimension2d<float>(ScreenSize.Width + 200.0f, ScreenSize.Height + 200.0f)));
    GameSpeed = 0;
    result = true;
    return result;
}

bool CPlayState::initializeNewGame()
{
    eraseGameObjects();
    StartTime = time(0);
    int random = ox::algo::CRand::rand();
    RandomValue = (random & 0xffffff) | (((random & 0xff00) >> 8) + ((random & 0xff0000) >> 16) + random) << 24;
    if (settings::gp_profileManager && settings::gp_profileManager->getCurrentProfile())
    {
        Profile = settings::gp_profileManager->getCurrentProfile();
        PlayerName = Profile->getPlayerName();
        PlayerGroup = Profile->getPlayerGroup();
    }
    if (settings::gp_systemConfig)
    {
        ParticleSetting = settings::gp_systemConfig->getParticleSetting();
        ScrollSpeed = settings::gp_systemConfig->getScrollSpeed();
    }

    GameMode = g_gameMode;
    GameTime = 0;
    Victory = false;
    LevelRecordShown = false;
    MineralsRecordShown = false;
    BuildingAttacked = false;
    if (GameMode == game::EGM_CAMPAIGN)
    {
        Scenario = new game::CScenario();
        g_gamePlanet = Scenario->getScenarioPlanet();
        int credits = Scenario->getStartingCredits();
        game::gp_world = new game::CWorld(GameMode, g_gamePlanet);
        game::createCredits();
        game::setCredits(credits);
        if (!game::gp_world->initializeWorld(Driver, getViewSize()))
            return false;

        entity::gp_entityManager = new entity::CEntityManager();
        entity::g_nextEntityId = 1;
        game::gp_world->initializeNewGame(Scenario);
        Scenario->addStartingEntities();
        Scenario->createScenarioEvents(this);
        if (StoryScreen)
        {
            StoryScreen->displayBlackness(10.0f);
            StoryScreen->setVisible(true);
        }
    }
    else
    {
        game::gp_world = new game::CWorld(GameMode, g_gamePlanet);
        game::createCredits();
        if (!game::gp_world->initializeWorld(Driver, getViewSize()))
            return false;

        entity::gp_entityManager = new entity::CEntityManager();
        entity::g_nextEntityId = 1;
        game::gp_world->initializeNewGame(0);
        entity::gp_entityManager->addBuilding(0, 510.0f, 510.0f);
        entity::gp_entityManager->addBuilding(0, 590.0f, 540.0f);
        entity::gp_entityManager->addBuilding(1, 530.0f, 550.0f);
        entity::gp_entityManager->addBuilding(1, 480.0f, 480.0f);
        entity::gp_entityManager->addBuilding(1, 570.0f, 460.0f);
        entity::gp_entityManager->appendEntity(new entity::CDropshipEntity(600.0f, 500.0f, true), 4);
        displayWelcomeMessage();
        if (GameMode == game::EGM_RUSH)
        {
            game::setCredits(1500);
            entity::gp_entityManager->addBuilding(0, 540.0f, 440.0f);
            entity::gp_entityManager->addBuilding(0, 620.0f, 470.0f);
        }
        else if (GameMode == game::EGM_INSANE)
        {
            game::setCredits(1000);
            entity::gp_entityManager->addBuilding(0, 540.0f, 440.0f);
            entity::gp_entityManager->addBuilding(0, 620.0f, 470.0f);
        }
        else
            game::setCredits(45);
    }

    ViewPosition.X = (game::gp_world->getActualGameFieldSize().getWidth() - ScreenSizeF.Width) * 0.5f;
    ViewPosition.Y = (game::gp_world->getActualGameFieldSize().getHeight() - ScreenSizeF.Height) * 0.5f;
    ThreatLevel = new game::CThreatLevel(GameMode);
    game::gp_statistics = new game::CStatistics();
    if (settings::gp_profileManager && settings::gp_profileManager->getCurrentProfile())
    {
        settings::CHarvestProfile* profile = settings::gp_profileManager->getCurrentProfile();
        for (int i = 0; i < 5; ++i)
        {
            settings::g_attackPriorities[i].setPrioritiesFromString(profile->getAttackPriority(i));
            settings::g_attackPriorities[i].setIfRangeIsImportant(profile->getAttackRangeMatters(i));
        }
    }

    if (GameMode == game::EGM_CREATIVE)
    {
        BuildableItems.loadCreativeBuildings(Device);
        if (!game::CLuaManager::s_availableLuaScriptFiles.empty())
        {
            entity::gp_entityManager->update(0.0f, ox::core::CRect<float>(0.0f, 0.0f, 1.0f, 1.0f));
            LuaManager = new game::CLuaManager(Device, this);
            LuaManager->initLuaByScriptList();
            LuaManager->hookNewGame();
            ox::TArray<ox::core::CString<char> >& errors = LuaManager->getCompilerErrors();
            if (!errors.empty())
                for (unsigned int i = 0; i < errors.size(); ++i)
                    addInfoLine(ox::core::CString<wchar_t>(errors[i].c_str()));
        }
    }
    return true;
}

void CPlayState::realignGui()
{
    ScreenSize = Device->getVideoDriver()->getScreenSize();
    ScreenSizeF.Height = ScreenSize.Height;
    ScreenSizeF.Width = ScreenSize.Width;
    ShowMinimap = false;
    int minimapSize = 0;
    // The minimap needs a screen larger than 800x600.
    int extraWidth = ScreenSize.Width - 800;
    int extraHeight = ScreenSize.Height - 600;
    if (extraWidth > 40 && extraHeight > 40 && GuiSprites[GS_MINIMAP_BACKGROUND])
    {
        ShowMinimap = true;
        minimapSize = (extraWidth > extraHeight ? extraHeight : extraWidth) + 100;
        ox::core::CDimension2d<int> backgroundSize = GuiSprites[GS_MINIMAP_BACKGROUND]->getFrameSize(0);
        if (minimapSize > backgroundSize.Height)
            minimapSize = backgroundSize.Height;
    }

    ox::core::CDimension2d<int> bottomLeftSize = GuiSprites[GS_BOTTOM_LEFT_BACKGROUND]->getFrameSize(0);
    ox::core::CDimension2d<int> bottomRightSize = GuiSprites[GS_BOTTOM_RIGHT_BACKGROUND]->getFrameSize(0);
    ox::core::CDimension2d<int> topLeftSize = GuiSprites[GS_TOP_LEFT_BACKGROUND]->getFrameSize(0);
    ox::core::CDimension2d<int> topRightSize = GuiSprites[GS_TOP_RIGHT_BACKGROUND]->getFrameSize(0);
    if (ShowMinimap)
    {
        MinimapWidth = GuiSprites[GS_MINIMAP_LEFT]->getFrameSize(0).Width + GuiSprites[GS_MINIMAP_RIGHT]->getFrameSize(0).Width +
            minimapSize;
        MinimapHeight = GuiSprites[GS_MINIMAP_TOP]->getFrameSize(0).Height + GuiSprites[GS_MINIMAP_BOTTOM]->getFrameSize(0).Height +
            minimapSize;
        topRightSize = ox::core::CDimension2d<int>(MinimapWidth, MinimapHeight);
        topRightSize.Width += GuiSprites[GS_MINERALS_BACKGROUND]->getFrameSize(0).Width;
    }
    TimerWidth = 0;
    if (displayTimerForGameMode(GameMode))
    {
        TimerWidth = GuiSprites[GS_TIME_BACKGROUND]->getFrameSize(0).Width;
        topRightSize.Width += TimerWidth;
    }
    if (displayThreatLevelForGameMode(GameMode))
        topRightSize.Width += GuiSprites[GS_THREAT_LEVEL_BACKGROUND]->getFrameSize(0).Width;

    BottomBar->setRelativePosition(
        ox::core::CRect<int>(0, ScreenSize.Height - bottomLeftSize.Height, ScreenSize.Width, ScreenSize.Height));
    ox::core::CRect<int> bar = BottomBar->getAbsolutePosition();
    int barWidth = bar.getWidth();
    int barHeight = bar.getHeight();
    BarLeftArea = ox::core::CRect<int>(bar.UpperLeftCorner, bottomLeftSize);
    ox::core::CPosition2d<int> rightCorner(bar.LowerRightCorner.X - bottomRightSize.Width,
        bar.LowerRightCorner.Y - bottomRightSize.Height);
    BarRightArea = ox::core::CRect<int>(rightCorner, bottomRightSize);
    BarCenterArea = ox::core::CRect<int>(BarLeftArea.LowerRightCorner.X, BarLeftArea.UpperLeftCorner.Y,
        BarRightArea.UpperLeftCorner.X, BarLeftArea.LowerRightCorner.Y);

    GuiElements[GUI_ID_PRIORITIES]->moveTo(ox::core::CPosition2d<int>(8, barHeight - 49));
    GuiElements[GUI_ID_SPEED_PAUSE]->moveTo(ox::core::CPosition2d<int>(8, barHeight - 23));
    GuiElements[GUI_ID_SPEED_SLOW]->moveTo(ox::core::CPosition2d<int>(46, barHeight - 23));
    GuiElements[GUI_ID_SPEED_NORMAL]->moveTo(ox::core::CPosition2d<int>(84, barHeight - 23));
    GuiElements[GUI_ID_SPEED_DOUBLE]->moveTo(ox::core::CPosition2d<int>(122, barHeight - 23));
    GuiElements[GUI_ID_SPEED_FOUR]->moveTo(ox::core::CPosition2d<int>(160, barHeight - 23));
    GuiElements[GUI_ID_BUILDINGS_LEFT]->moveTo(ox::core::CPosition2d<int>(279, barHeight - 46));
    GuiElements[GUI_ID_BUILDINGS_RIGHT]->moveTo(ox::core::CPosition2d<int>(barWidth - 161, barHeight - 46));
    GuiElements[GUI_ID_MENU]->moveTo(ox::core::CPosition2d<int>(barWidth - 106, barHeight - 46));
    RecycleButton->moveTo(ox::core::CPosition2d<int>(226, barHeight - 41));

    ox::core::CRect<int> buildingsRect(311, barHeight - 46, barWidth - 166, barHeight - 4);
    BuildingsArea->setRelativePosition(buildingsRect);
    BuildingsList->removeAllChildren();
    BuildingsList->setRelativePosition(ox::core::CRect<int>(0, 0, 100, 50));
    for (int i = 0; i < BuildableItems.getNumBuildings(); ++i)
    {
        entity::SBuildingInfoItem* info = BuildableItems.getBuildingInfo(i);
        if (info && info->Enabled)
        {
            ox::gui::IGUILayout* layout =
                GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 60, 42), BuildingsList);
            layout->setHoverItem(getPopupForBuildButton(info->Name.c_str(), info->Description.c_str(),
                info->SparkCost.getValue(), info->MineralCost.getValue()));
            BuildableItems.setItemButtonLayout(i, layout);
        }
        else
            BuildableItems.setItemButtonLayout(i, 0);
    }
    BuildingsList->sortHorizontally(1);
    int listWidth = BuildingsList->getRelativePosition().getWidth();
    int space = buildingsRect.getWidth();
    if (listWidth < space)
    {
        BuildingsList->moveTo(ox::core::CPosition2d<int>((space - listWidth) / 2, 0));
        GuiElements[GUI_ID_BUILDINGS_LEFT]->setEnabled(false);
        GuiElements[GUI_ID_BUILDINGS_RIGHT]->setEnabled(false);
    }
    else
    {
        BuildingsList->moveTo(ox::core::CPosition2d<int>(0, 0));
        GuiElements[GUI_ID_BUILDINGS_LEFT]->setEnabled(true);
        GuiElements[GUI_ID_BUILDINGS_RIGHT]->setEnabled(true);
    }

    BuildingsScrolling = false;
    ActionPanel->setRelativePosition(ox::core::CRect<int>(0, 0, topLeftSize.Width, topLeftSize.Height + 120));
    InfoText->setRelativePosition(ox::core::CRect<int>(115, 5, 392, 27));
    SelectedNameText->setRelativePosition(ox::core::CRect<int>(7, 6, 107, 18));
    OperatorText->setRelativePosition(ox::core::CRect<int>(7, 20, 107, 30));
    MiniStatText->setRelativePosition(ox::core::CRect<int>(7, 69, 107, 79));
    GuiElements[GUI_ID_DESELECT]->moveTo(ox::core::CPosition2d<int>(6, 86));
    GuiElements[GUI_ID_UNLINK]->moveTo(ox::core::CPosition2d<int>(40, 86));
    GuiElements[GUI_ID_OVERCHARGE]->moveTo(ox::core::CPosition2d<int>(74, 86));
    GuiElements[GUI_ID_EAGLE]->moveTo(ox::core::CPosition2d<int>(40, 86));
    GuiElements[GUI_ID_TEMPEST]->moveTo(ox::core::CPosition2d<int>(74, 86));
    GuiElements[GUI_ID_DEATHSTAR]->moveTo(ox::core::CPosition2d<int>(74, 86));
    GuiElements[GUI_ID_UNLINK_DEATHSTAR]->moveTo(ox::core::CPosition2d<int>(74, 86));
    GuiElements[GUI_ID_END_LASER]->moveTo(ox::core::CPosition2d<int>(6, 121));
    GuiElements[GUI_ID_SPEED_BUILD]->moveTo(ox::core::CPosition2d<int>(40, 86));
    GuiElements[GUI_ID_UNLINK_SPEED_BUILD]->moveTo(ox::core::CPosition2d<int>(40, 86));
    GuiElements[GUI_ID_SELL_HARVESTERS]->moveTo(ox::core::CPosition2d<int>(40, 86));
    GuiElements[GUI_ID_REPLACE_PRODUCER]->moveTo(ox::core::CPosition2d<int>(40, 86));

    TopBar->setRelativePosition(
        ox::core::CRect<int>(ScreenSize.Width - topRightSize.Width, 0, ScreenSize.Width, topRightSize.Height));
    ox::core::CRect<int> top = TopBar->getAbsolutePosition();
    if (ShowMinimap)
    {
        int x = top.LowerRightCorner.X - MinimapWidth + (MinimapWidth - minimapSize) / 2;
        int y = top.UpperLeftCorner.Y + GuiSprites[GS_MINIMAP_TOP]->getFrameSize(0).Height;
        MinimapRect = ox::core::CRect<int>(x, y, x + minimapSize, y + minimapSize);
        int right = topRightSize.Width - MinimapWidth;
        CreditsText->setRelativePosition(ox::core::CRect<int>(right - 87, 5, right - 41, 27));
        right = topRightSize.Width - MinimapWidth;
        HarvestersText->setRelativePosition(ox::core::CRect<int>(right - 41, 5, right - 11, 27));
        right = topRightSize.Width - MinimapWidth - GuiSprites[GS_MINERALS_BACKGROUND]->getFrameSize(0).Width;
        ThreatLevelText->setRelativePosition(ox::core::CRect<int>(right - 53, 5, right - 27, 27));
    }
    else
    {
        MinimapRect = ox::core::CRect<int>(top.LowerRightCorner.X - 105, top.UpperLeftCorner.Y + 14,
            top.LowerRightCorner.X - 6, top.UpperLeftCorner.Y + 115);
        CreditsText->setRelativePosition(ox::core::CRect<int>(topRightSize.Width - 193, 5, topRightSize.Width - 147, 27));
        HarvestersText->setRelativePosition(ox::core::CRect<int>(topRightSize.Width - 147, 5, topRightSize.Width - 117, 27));
        ThreatLevelText->setRelativePosition(ox::core::CRect<int>(topRightSize.Width - 289, 5, topRightSize.Width - 252, 27));
    }

    if (game::gp_world)
        game::gp_world->changeViewSize(getViewSize());
    if (!displayThreatLevelForGameMode(GameMode))
        ThreatLevelText->setVisible(false);

    if (GameMode == game::EGM_WAVE)
    {
        toggleWaveList();
        toggleWaveList();
    }
    else if (GameMode == game::EGM_CREATIVE && LuaManager)
    {
        if (LuaManager->isCreativeListVisible())
        {
            toggleCreativeList();
            toggleCreativeList();
        }
        else if (ListGroups[LIST_CREATIVE])
        {
            ListGroups[LIST_CREATIVE]->remove();
            ListGroups[LIST_CREATIVE] = 0;
        }

        if (LuaManager->isWaveListVisible())
        {
            toggleWaveList();
            toggleWaveList();
        }
        else if (ListGroups[LIST_WAVES])
        {
            ListGroups[LIST_WAVES]->remove();
            ListGroups[LIST_WAVES] = 0;
        }
    }

    if (m_7b0)
        m_7b0->setRelativePosition(
            ox::core::CRect<int>(150, ScreenSize.Height - 100, ScreenSize.Width - 150, ScreenSize.Height - 85));
}

void CPlayState::playPlanetMusic()
{
    if (game::gp_world && GameMode != game::EGM_CAMPAIGN)
    {
        switch (game::gp_world->getPlanet())
        {
        case 0:
            AudioDriver->playMusic("mus_hephaestus.ogg", 1.0f, false);
            break;
        case 1:
            AudioDriver->playMusic("mus_poseidon.ogg", 1.0f, false);
            break;
        case 2:
            AudioDriver->playMusic("mus_ares.ogg", 1.0f, false);
            break;
        }
    }
    MusicTime = 1200.0f;
}

int CPlayState::updateState(float time)
{
    if (!Device->run() || !Driver)
        return 1;

    unsigned int startTime = Device->getTimer()->getTime();
    if (m_keys[ox::KEY_DOWN])
    {
        ViewPosition.Y += 200.0f * time * ScrollSpeed;
        CursorMoved = true;
    }
    if (m_keys[ox::KEY_UP])
    {
        ViewPosition.Y += -200.0f * time * ScrollSpeed;
        CursorMoved = true;
    }
    if (m_keys[ox::KEY_LEFT])
    {
        ViewPosition.X += -200.0f * time * ScrollSpeed;
        CursorMoved = true;
    }
    if (m_keys[ox::KEY_RIGHT])
    {
        ViewPosition.X += 200.0f * time * ScrollSpeed;
        CursorMoved = true;
    }
    // In fullscreen the view scrolls when the mouse touches the screen edges.
    if (Driver->isFullscreen() && Follow.Id < 0)
    {
        if (MousePosition.X == 0)
            ViewPosition.X += -400.0f * time * ScrollSpeed;
        else if (MousePosition.X >= ScreenSize.Width - 1)
            ViewPosition.X += 400.0f * time * ScrollSpeed;
        if (MousePosition.Y == 0)
            ViewPosition.Y += -400.0f * time * ScrollSpeed;
        else if (MousePosition.Y >= ScreenSize.Height - 1)
            ViewPosition.Y += 400.0f * time * ScrollSpeed;
    }
    if (game::gp_world)
        game::gp_world->constrainViewPos(ViewPosition);

    if (BuildingsScrolling)
    {
        BuildingsScrollPosition += (BuildingsScrollTarget - BuildingsScrollPosition) * time * 5.0f;
        BuildingsList->moveTo(ox::core::CPosition2d<int>((int)BuildingsScrollPosition, 0));
    }

    DenialTime -= time;
    MinimapUpdateTime -= time;
    MusicTime -= time;
    if (MusicTime < 0)
        playPlanetMusic();

    // The game steps at most 0.06 seconds per frame, or 0.5 seconds at the fastest speeds.
    float frameDelta;
    if (GameSpeed > 4)
        frameDelta = ox::core::clamp(time * GAME_SPEED_MULTIPLIERS[GameSpeed], 0.0f, 0.5f);
    else
        frameDelta = ox::core::clamp(time * GAME_SPEED_MULTIPLIERS[GameSpeed], 0.0f, 0.06f);
    switch (GameMode)
    {
    case game::EGM_WAVE:
        if (!Victory && !GameOver)
        {
            float previousTime = GameTime;
            GameTime += frameDelta;
            if (entity::gp_entityManager->getNumAliens() <= 0 && ThreatLevel->getThreatLevel() >= 10 &&
                ThreatLevel->allowVictory())
            {
                Victory = true;
                displayTimeVictoryMessage();
                GameOver = true;
                GameWon = true;
                GameOverTime = 15.0f;
                g_scenarioResult = 3;
                g_scenarioResultGameMode = GameMode;
                g_scenarioResultPlanet = game::gp_world->getPlanet();
                if (game::gp_statistics)
                    game::gp_statistics->reportNewThreatLevel((int)GameTime / 300 + 1, GameTime);
                sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 6);
                if (GameTime < 3600.0f)
                    sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 7);
            }
            else if ((int)previousTime / 300 != (int)GameTime / 300)
                game::gp_statistics->reportNewThreatLevel((int)GameTime / 300, GameTime);
        }
        break;
    case game::EGM_RUSH:
        if (!Victory && !GameOver)
        {
            float previousTime = GameTime;
            GameTime += frameDelta;
            if (game::gp_statistics && game::gp_statistics->getRushModeDamage() >= 50000.0f)
            {
                Victory = true;
                displayTimeVictoryMessage();
                GameOver = true;
                GameWon = true;
                GameOverTime = 15.0f;
                g_scenarioResult = 3;
                g_scenarioResultGameMode = GameMode;
                g_scenarioResultPlanet = game::gp_world->getPlanet();
                if (game::gp_statistics)
                    game::gp_statistics->reportNewThreatLevel((int)GameTime / 60 + 1, GameTime);
                sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 4);
                sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 5);
                if (!BuildingAttacked)
                    sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 13);
            }
            else if ((int)previousTime / 60 != (int)GameTime / 60)
                game::gp_statistics->reportNewThreatLevel((int)GameTime / 60, GameTime);
        }
        break;
    case game::EGM_CREATIVE:
    {
        float previousTime = GameTime;
        GameTime += frameDelta;
        if (game::gp_statistics && (int)previousTime / 300 != (int)GameTime / 300)
            game::gp_statistics->reportNewThreatLevel((int)GameTime / 300 + 1, GameTime);
        if (LuaManager)
        {
            LuaManager->setViewPosition(ViewPosition);
            LuaManager->runFrameFunctions(frameDelta);
        }
        break;
    }
    default:
        GameTime += frameDelta;
        break;
    }

    // Keep the local records of the normal and insane games up to date.
    RecordCheckTime -= frameDelta;
    if ((GameMode == game::EGM_NORMAL || GameMode == game::EGM_INSANE) && RecordCheckTime <= 0)
    {
        RecordCheckTime = 5.0f;
        if (PlayerName == settings::gp_profileManager->getCurrentProfile()->getPlayerName())
        {
            int levelRecord = settings::gp_profileManager->getCurrentProfile()->getLocalScore(0, GameMode,
                game::gp_world->getPlanet());
            int level = ThreatLevel->getThreatLevel();
            if (level > levelRecord)
            {
                settings::gp_profileManager->getCurrentProfile()->updateLocalScore(0, GameMode,
                    game::gp_world->getPlanet(), level);
                if (!LevelRecordShown)
                {
                    displayRecordMessage(level, false);
                    LevelRecordShown = true;
                }
            }
            int mineralsRecord = settings::gp_profileManager->getCurrentProfile()->getLocalScore(1, GameMode,
                game::gp_world->getPlanet());
            int minerals = game::gp_statistics->getGameStatValue(1);
            if (minerals > mineralsRecord)
            {
                settings::gp_profileManager->getCurrentProfile()->updateLocalScore(1, GameMode,
                    game::gp_world->getPlanet(), minerals);
                if (!MineralsRecordShown)
                {
                    displayRecordMessage(minerals, true);
                    MineralsRecordShown = true;
                }
            }
        }
    }

    if (Scenario)
        Scenario->update(frameDelta);
    if (StoryScreen)
        StoryScreen->update(frameDelta);
    if (AchievementsScreen && AchievementsScreen->isVisible())
        AchievementsScreen->update(time);
    if (HighlightActive)
    {
        HighlightTime -= frameDelta;
        if (HighlightTime <= 0)
            HighlightActive = false;
    }

    entity::CEntity::g_screenSizeF = ScreenSizeF;
    entity::CEntity::g_screenCenterPos =
        ox::core::CPosition2d<float>(ScreenSizeF.Width * 0.5f + ViewPosition.X, ScreenSizeF.Height * 0.5f + ViewPosition.Y);

    // Repair the credits now and then if they were edited in memory.
    if (ox::algo::CRand::rand() % 10 == 0 && game::gp_mineralAmount && game::gp_negatedMineralAmount)
    {
        int credits = game::gp_mineralAmount->getValue();
        int negated = -game::gp_negatedMineralAmount->getValue();
        if (credits != negated)
        {
            game::gp_mineralAmount->setValue(credits < negated ? credits : negated);
            game::gp_negatedMineralAmount->setValue(-game::gp_mineralAmount->getValue());
        }
    }

    for (std::list<SMinimapMarker>::iterator it = MinimapMarkers.begin(); it != MinimapMarkers.end();)
    {
        it->Time -= frameDelta;
        if (it->Time <= 0)
            it = MinimapMarkers.erase(it);
        else
            ++it;
    }

    float remaining = frameDelta;
    do
    {
        float step = ox::core::clamp(remaining, 0.0f, 0.06f);
        if (ThreatLevel && game::gp_world && !GameOver && ThreatLevel->update(step))
        {
            if (game::gp_statistics)
                game::gp_statistics->reportNewThreatLevel(ThreatLevel->getThreatLevel(), GameTime);
            // Insane games pay a bonus every ten threat levels.
            if (GameMode == game::EGM_INSANE && ThreatLevel->getThreatLevel() % 10 == 0)
            {
                game::gp_mineralAmount->modifyValue(250);
                game::gp_negatedMineralAmount->modifyValue(-250);
                displayInsaneRewardMessage();
            }
        }

        if (entity::gp_entityManager)
        {
            VisibleArea = ox::core::CRect<float>(ViewPosition + ox::core::CPosition2d<float>(-100.0f, -100.0f),
                ox::core::CDimension2d<float>(ScreenSize.Width + 200.0f, ScreenSize.Height + 200.0f));
            entity::gp_entityManager->update(step, VisibleArea);
            Minerals = game::gp_mineralAmount->getValue();
            if (game::gp_statistics)
            {
                game::gp_statistics->setHighest(1, entity::gp_entityManager->getNumBuildings());
                game::gp_statistics->setHighest(0, game::gp_mineralAmount->getValue());
            }

            if (SelectedEntity)
            {
                SelectedEntity = entity::gp_entityManager->updateClickableReference(SelectedEntityId, SelectedEntity);
                if (SelectedEntity)
                {
                    if (SelectedEntity->getEntityType() != SelectedEntityType)
                        newSelectedEntity();
                    InfoText->setText(SelectedEntity->getInfoString().c_str());
                    OperatorText->setText(SelectedEntity->getOperatorString().c_str());
                    MiniStatText->setText(SelectedEntity->getMiniStatString().c_str());
                }
                else
                    clearSelectedEntity();
            }
            updateMultiSelectionReferences();
            if (LuaManager)
                LuaManager->updateSelectedBuildings(&MultiSelection);
            if (RecycleTarget)
                RecycleTarget = entity::gp_entityManager->updateClickableReference(RecycleTargetId, RecycleTarget);

            if (Follow.Id >= 0)
            {
                entity::gp_entityManager->updateReference(Follow, FollowLayer, true);
                ox::entity::COxEntity* entity = Follow.Entity;
                if (entity)
                {
                    ox::core::CPosition2d<float> position(entity->getPosition().X,
                        entity->getPosition().Y - entity->getPosition().Z);
                    ox::core::CPosition2d<float> target =
                        position - ox::core::CPosition2d<float>(ScreenSizeF.Width * 0.5f, ScreenSizeF.Height * 0.5f);
                    if (FollowJump)
                    {
                        ViewPosition = target;
                        FollowJump = false;
                    }
                    else
                        ViewPosition += ox::core::CPosition2d<float>((target.X - ViewPosition.X) * 4.0f * step,
                            (target.Y - ViewPosition.Y) * 4.0f * step);
                    game::gp_world->constrainViewPos(ViewPosition);
                }
            }

            if (entity::gp_entityManager->getNumBuildings() <= 0 && !GameOver)
            {
                GameOver = true;
                GameWon = false;
                GameOverTime = 15.0f;
                g_scenarioResult = 4;
                g_scenarioResultGameMode = GameMode;
                g_scenarioResultPlanet = game::gp_world->getPlanet();
                if (game::gp_statistics)
                    game::gp_statistics->reportNewThreatLevel(ThreatLevel->getThreatLevel() + 1, GameTime);
            }
        }

        if (Selector)
            Selector->update(step);
        if (game::gp_world)
            game::gp_world->update(step, entity::gp_entityManager->getBuildingsBoundingBox());
        if (entity::gp_entityManager->getNumAliens() == 0 && GameMode == game::EGM_WAVE)
            checkWaveReward();
        remaining -= step;
    } while (remaining > 0);

    if (GameMode == game::EGM_NORMAL)
    {
        if (game::gp_statistics->getGameStatValue(2) == 1)
            sendCustomEvent(ECE_TUTORIAL_HINT, 26);
        if (ThreatLevel->getThreatLevel() == 11)
            sendCustomEvent(ECE_TUTORIAL_HINT, 30);
        if (ThreatLevel->getThreatLevel() == 50)
            sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 1);
        if (ThreatLevel->getThreatLevel() == 100)
        {
            sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 2);
            sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 3);
        }
        if (ThreatLevel->getThreatLevel() == 15 && !BuildingAttacked && game::gp_world->getPlanet() == 0)
            sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 0);
        if (ThreatLevel->getThreatLevel() <= 10 && HarvestingCount >= 20)
            sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 17);
        if (HarvestingCount >= 50)
            sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 12);
        if (OverheatedCount >= 50)
            sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 18);
    }
    else if (GameMode == game::EGM_INSANE)
    {
        if (ThreatLevel->getThreatLevel() == 50)
        {
            sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 9);
            sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 8);
        }
    }
    else if (GameMode == game::EGM_RUSH)
    {
        if (game::gp_statistics->getRushModeDamage() > 0 && game::gp_statistics->getRushModeDamage() < 510.0f)
            sendCustomEvent(ECE_TUTORIAL_HINT, 32);
    }

    if (GameOver)
    {
        GameOverTime -= time;
        if (GameOverTime <= 0)
        {
            NextState = 3;
            if (GameMode != game::EGM_CAMPAIGN)
            {
                bool highscore;
                if (GameWon)
                    highscore = GameMode != game::EGM_CREATIVE;
                else
                    highscore = GameMode == game::EGM_NORMAL || GameMode == game::EGM_INSANE;
                if (highscore)
                    game::CHighscoreInfo::setNewHighscoreInfo(new game::CHighscoreInfo(PlayerName.c_str(),
                        PlayerGroup.c_str(), RandomValue, StartTime, GameMode, game::gp_world->getPlanet(),
                        game::gp_statistics->getGameStatValue(1), ThreatLevel->getThreatLevel(), GameTime));
                else
                    game::CHighscoreInfo::setNewHighscoreInfo(0);
            }
        }
    }

    if (InfoLines)
        InfoLines->update(time, BarLeftArea.UpperLeftCorner.Y);

    StatsTime -= time;
    if (StatsTime < 0)
    {
        StatsTime = 1.0f;
        HarvestingCount = 0;
        OverheatedCount = 0;
        const std::list<ox::entity::COxEntity*>& buildings = entity::gp_entityManager->getEntityList(0);
        for (std::list<ox::entity::COxEntity*>::const_iterator it = buildings.begin(); it != buildings.end(); ++it)
        {
            if ((*it)->getEntityType() == 4)
            {
                if (((entity::CMineralGatherEntity*)*it)->isAbleToHarvest())
                    ++HarvestingCount;
            }
            else if ((*it)->getEntityType() == 1 && ((entity::CSparkMoverEntity*)*it)->isOverheated())
                ++OverheatedCount;
        }
        if (HarvestersText)
        {
            ox::core::CString<wchar_t> text(L"+");
            text.append(HarvestingCount);
            HarvestersText->setText(text.c_str());
        }
    }

    if (Action == 1)
    {
        if (CursorMoved)
        {
            updatePlacementPosition();
            // Dragging places a chain of buildings a link's reach apart.
            if (HasLastPlacement && PlacementOk)
            {
                float distance = ox::core::CMath::getExactDistance(PlacementPosition, LastPlacement);
                if (distance > 140.0f && distance < 150.0f)
                    buyBuildingAtPlacementPos();
            }
        }
    }
    else if (Action == 2 && CursorMoved)
        updateRecycleBuilding();

    // Spawn fewer particles while the frame rate is low.
    int fps = Driver->getFPS();
    int quality = entity::CEntity::gp_particlePackage->ImportanceLevel;
    if (fps > 54)
        quality = 2;
    else if (fps >= 30 && fps < 40)
        quality = 1;
    else if (fps < 20)
        quality = 0;
    if (quality > ParticleSetting)
        quality = ParticleSetting;
    entity::CEntity::gp_particlePackage->ImportanceLevel = quality;

    if (!SelectedEntity)
        ActionPanel->setVisible(false);
    else if (ActionPanel)
    {
        int type = SelectedEntity->getEntityType();
        GuiElements[GUI_ID_DEATHSTAR]->setVisible(
            type == 7 && ((entity::CDefenseTowerEntity*)SelectedEntity)->getNumBackTargets() == 0);
        GuiElements[GUI_ID_UNLINK_DEATHSTAR]->setVisible(
            type == 7 && ((entity::CDefenseTowerEntity*)SelectedEntity)->getNumBackTargets() != 0);
        GuiElements[GUI_ID_END_LASER]->setVisible(type == 7 && ((entity::CDefenseTowerEntity*)SelectedEntity)->isLinked());
        GuiElements[GUI_ID_SPEED_BUILD]->setVisible(
            type == 3 && !((entity::CConstructionEntity*)SelectedEntity)->haveMoversBeenCalled());
        GuiElements[GUI_ID_UNLINK_SPEED_BUILD]->setVisible(
            type == 3 && ((entity::CConstructionEntity*)SelectedEntity)->haveMoversBeenCalled());
        GuiElements[GUI_ID_SELL_HARVESTERS]->setVisible(
            type == 4 && !((entity::CMineralGatherEntity*)SelectedEntity)->hasMoreMinerals());
        GuiElements[GUI_ID_REPLACE_PRODUCER]->setVisible(
            type == 0 && ((entity::CSparkProducerEntity*)SelectedEntity)->isExpired());
    }

    UpdateDuration = Device->getTimer()->getTime() - startTime;
    return NextState;
}

void CPlayState::displayTimeVictoryMessage()
{
    int record = settings::gp_profileManager->getCurrentProfile()->getLocalScore(2, GameMode,
        game::gp_world->getPlanet());
    int time = (int)(GameTime * 1000.0f);

    game::SInfoLineMessage message;
    message.Name = settings::gp_systemConfig->getLocalizedText(L"ingame:victoryTitle");
    message.Portrait = "PortraitCommunications";
    message.Text = settings::gp_systemConfig->getLocalizedText(L"ingame:victory");
    message.Sound = "ingame_infoVictory.ogg";
    if (time < record || record == 0)
    {
        settings::gp_profileManager->getCurrentProfile()->updateLocalScore(2, GameMode,
            game::gp_world->getPlanet(), time);
        message.Text = settings::gp_systemConfig->getLocalizedText(L"ingame:victoryNewRecord",
            ox::core::CStringFunctions::millisecondsToWide(GameTime, true).c_str());
    }
    addInfoLine(&message, false);
}

void CPlayState::displayRecordMessage(int value, bool minerals)
{
    game::SInfoLineMessage message;
    message.Name = settings::gp_systemConfig->getLocalizedText(L"ingame:recordTitle");
    message.Portrait = "PortraitCommunications";
    if (minerals)
        message.Text = settings::gp_systemConfig->getLocalizedText(L"ingame:newMineralsRecord", value);
    else
        message.Text = settings::gp_systemConfig->getLocalizedText(L"ingame:newLevelRecord", value);
    addInfoLine(&message, false);
}

void CPlayState::displayInsaneRewardMessage()
{
    game::SInfoLineMessage message;
    message.Name = settings::gp_systemConfig->getLocalizedText(L"ingame:insaneRewardTitle");
    message.Portrait = "PortraitCommunications";
    message.Text = settings::gp_systemConfig->getLocalizedText(L"ingame:insaneRewardMessage", 250);
    addInfoLine(&message, false);
}

void CPlayState::clearSelectedEntity()
{
    SelectedEntity = 0;
    MultiSelection.clear();
    if (InfoText)
        InfoText->setText(L"");
    if (SelectedNameText)
        SelectedNameText->setText(L"");
    if (OperatorText)
        OperatorText->setText(L"");
    if (MiniStatText)
        MiniStatText->setText(L"");
    if (ActionPanel)
        ActionPanel->setVisible(false);
    if (LuaManager)
        LuaManager->updateSelectedBuilding(0);
}

void CPlayState::newSelectedEntity()
{
    for (ox::TArray<ox::entity::SEntityReference*>::iterator it = MultiSelection.begin(); it != MultiSelection.end(); ++it)
        delete *it;
    MultiSelection.clear();

    if (!SelectedEntity)
    {
        clearSelectedEntity();
        return;
    }

    SelectedEntityType = SelectedEntity->getEntityType();
    if (SelectedNameText)
    {
        ox::core::CString<wchar_t> name;
        if (SelectedEntity->getEntityType() != 16)
            name = settings::gp_systemConfig->getLocalizedText(
                entity::ENTITY_KEY_NAMES[SelectedEntity->getEntityType()]).c_str();
        else
            name = ((entity::CCreativeEntity*)SelectedEntity)->getBuildingName();
        if (BoldFont->getDimension(name.c_str()).Width > 100)
            SelectedNameText->setOverrideFont(SmallFont);
        else
            SelectedNameText->setOverrideFont(BoldFont);
        SelectedNameText->setText(name.c_str());
        SelectedNameText->activateOffsetScrollingToEnsureVisibleText();
    }

    if (ActionPanel)
    {
        ActionPanel->setVisible(true);
        int type = SelectedEntity->getEntityType();
        GuiElements[GUI_ID_DESELECT]->setVisible(true);
        bool linker = type == 1;
        bool tower = type == 7;
        GuiElements[GUI_ID_UNLINK]->setVisible(tower | linker);
        GuiElements[GUI_ID_OVERCHARGE]->setVisible(linker);
        bool turret = type == 8;
        GuiElements[GUI_ID_EAGLE]->setVisible(turret);
        GuiElements[GUI_ID_TEMPEST]->setVisible(turret);
        GuiElements[GUI_ID_DEATHSTAR]->setVisible(
            tower && ((entity::CDefenseTowerEntity*)SelectedEntity)->getNumBackTargets() == 0);
        GuiElements[GUI_ID_UNLINK_DEATHSTAR]->setVisible(
            tower && ((entity::CDefenseTowerEntity*)SelectedEntity)->getNumBackTargets() != 0);
        GuiElements[GUI_ID_END_LASER]->setVisible(tower && ((entity::CDefenseTowerEntity*)SelectedEntity)->isLinked());
        GuiElements[GUI_ID_SPEED_BUILD]->setVisible(
            type == 3 && !((entity::CConstructionEntity*)SelectedEntity)->haveMoversBeenCalled());
        GuiElements[GUI_ID_UNLINK_SPEED_BUILD]->setVisible(
            type == 3 && ((entity::CConstructionEntity*)SelectedEntity)->haveMoversBeenCalled());
        GuiElements[GUI_ID_SELL_HARVESTERS]->setVisible(
            type == 4 && !((entity::CMineralGatherEntity*)SelectedEntity)->hasMoreMinerals());
        GuiElements[GUI_ID_REPLACE_PRODUCER]->setVisible(
            type == 0 && ((entity::CSparkProducerEntity*)SelectedEntity)->isExpired());
        if (linker)
            ((ox::gui::IGUIButton*)GuiElements[GUI_ID_UNLINK])->setAnimations(IngamePackage, "BtnActionUnlink", true);
        else if (tower)
            ((ox::gui::IGUIButton*)GuiElements[GUI_ID_UNLINK])->setAnimations(IngamePackage, "BtnActionUnlinkLaser",
                true);

        for (int i = GUI_ID_FIRST_LUA_ACTION; i < GUI_ID_WAVE_SEND; ++i)
        {
            if (GuiElements[i])
            {
                GuiElements[i]->remove();
                GuiElements[i] = 0;
            }
        }

        if (LuaManager && SelectedEntity->getEntityType() != 5)
        {
            // The script buttons follow the building's own buttons in a grid of three columns.
            int slot = 4;
            if (!tower)
            {
                slot = 3;
                if (!turret && !linker)
                {
                    slot = 1;
                    if (type == 0 || type == 4)
                        slot = 2;
                }
            }

            const char* buildingType = ((entity::CBuildingEntity*)SelectedEntity)->getBuildingType();
            game::SLuaEntityActionButton* action = 0;
            for (int id = GUI_ID_FIRST_LUA_ACTION; id < GUI_ID_WAVE_SEND; ++id, ++slot)
            {
                action = LuaManager->getNextEntityActionButton(buildingType, action);
                if (!action)
                    break;

                action->Id = id - GUI_ID_FIRST_LUA_ACTION;
                GuiElements[id] = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), ActionPanel, id, 0);
                ((ox::gui::IGUIButton*)GuiElements[id])->setAnimations(IngamePackage, "BtnActionDeselect", true);
                GuiElements[id]->moveTo(ox::core::CPosition2d<int>(slot % 3 * 34 + 6, slot / 3 * 35 + 86));
                if (action->Upgrade)
                {
                    entity::SBuildingInfoItem* info =
                        entity::gp_buildableItems->getBuildingInfoByEntityId(action->Command.c_str());
                    if (info)
                        GuiElements[id]->setHoverItem(getPopupForBuildButton(info->Name.c_str(),
                            info->Description.c_str(), info->SparkCost.getValue(), info->MineralCost.getValue()));
                }
                else
                    GuiElements[id]->setHoverItem(
                        getPopupForGuiButton(ox::core::CString<wchar_t>(action->Command.c_str()).c_str()));
            }
        }
    }

    if (LuaManager)
        LuaManager->updateSelectedBuilding(SelectedEntity);
}

void CPlayState::updateMultiSelectionReferences()
{
    for (ox::TArray<ox::entity::SEntityReference*>::iterator it = MultiSelection.begin(); it != MultiSelection.end();)
    {
        entity::gp_entityManager->updateReference(**it, 0, true);
        if ((*it)->Entity)
            ++it;
        else
        {
            delete *it;
            it = MultiSelection.erase(it);
        }
    }

    if (MultiSelection.size() == 1)
    {
        SelectedEntity = (entity::CEntity*)MultiSelection[0]->Entity;
        newSelectedEntity();
    }
}

void CPlayState::checkWaveReward()
{
    int reward = ThreatLevel->getWaveReward();
    if (reward <= 0 || GameOver)
        return;

    game::SInfoLineMessage message;
    message.Name = settings::gp_systemConfig->getLocalizedText(L"ingame:waveRewardTitle");
    message.Portrait = "PortraitCommunications";
    message.Text = settings::gp_systemConfig->getLocalizedText(L"ingame:waveRewardMessage", reward);
    addInfoLine(&message, false);

    game::gp_mineralAmount->modifyValue(reward);
    game::gp_negatedMineralAmount->modifyValue(-reward);
    if (game::gp_statistics)
        game::gp_statistics->addLog(GameTime, 0, reward);

    sendCustomEvent(ECE_TUTORIAL_HINT, 31);
    if (reward >= 16250)
        sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 19);
    if (!BuildingAttacked)
        sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 23);
    if (GameTime < 900.0f)
        sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 22);
}

void CPlayState::updatePlacementPosition()
{
    CursorMoved = false;
    PlacementPosition = getWorldPos(GUIEnvironment->getMousePosition());
    PlacementOk = false;
    if (HasLastPlacement && ox::core::CMath::getSquaredDistance(LastPlacement, PlacementPosition) > 22500.0f)
    {
        float angle = ox::core::CMath::getAngleIY(LastPlacement, PlacementPosition);
        PlacementPosition.X = LastPlacement.X + cos((double)angle) * 149.0;
        PlacementPosition.Y = LastPlacement.Y + sin((double)angle) * 149.0;
    }
    if (game::gp_world->mayPlaceObjectHere(PlacementPosition, false) &&
        entity::gp_entityManager->isBuildingPlacementOk(PlacementPosition,
            BuildableItems.getEntityRadius(BuildSelection)))
    {
        PlacementOk = true;
        m_2a0 = true;
    }
    if (!PlacementOk)
        PlacementPosition = getWorldPos(GUIEnvironment->getMousePosition());
}

void CPlayState::buyBuildingAtPlacementPos()
{
    if (game::gp_mineralAmount->getValue() < BuildableItems.getEntityMineralCost(BuildSelection))
    {
        AudioDriver->playSound("BtnDenial.ogg", 1.0f, 0.0f, 1.0f);
        DenialTime = 3.0f;
    }
    else if (PlacementOk)
    {
        int cost = BuildableItems.getEntityMineralCost(BuildSelection);
        game::gp_mineralAmount->modifyValue(-cost);
        game::gp_negatedMineralAmount->modifyValue(cost);
        const char* entityId = BuildableItems.getEntityId(BuildSelection);
        entity::CConstructionEntity* construction =
            new entity::CConstructionEntity(PlacementPosition.X, PlacementPosition.Y, entityId);
        entity::gp_entityManager->appendEntity(construction, 0);
        PlacementOk = false;
        m_2a0 = false;
        HasLastPlacement = true;
        LastPlacement = PlacementPosition;
        AudioDriver->playSound("PlaceBuilding.ogg", 1.0f, 0.0f, 1.0f);
        if (LuaManager)
            LuaManager->hookBuildingPlaced(construction, entityId);
    }
}

void CPlayState::updateRecycleBuilding()
{
    CursorMoved = false;
    ox::core::CPosition2d<float> position = getWorldPos(GUIEnvironment->getMousePosition());
    RecycleTarget = 0;
    RecycleTarget = entity::gp_entityManager->findClickableEntity(position);
    if (RecycleTarget)
    {
        // Minerals cannot be recycled.
        if (RecycleTarget->getEntityType() == 5)
            RecycleTarget = 0;
        else if (RecycleTarget)
            RecycleTargetId = RecycleTarget->getId();
    }
}

bool CPlayState::OnEvent(const ox::event::SEvent& event)
{
    // The open screens take the events first.
    if (PriorityScreen && PriorityScreen->isVisible() && PriorityScreen->OnEvent(event))
        return true;
    if (IngameMenuScreen && IngameMenuScreen->isVisible() && IngameMenuScreen->OnEvent(event))
        return true;
    if (SaveGameScreen && SaveGameScreen->isVisible() && SaveGameScreen->OnEvent(event))
        return true;
    if (StoryScreen && StoryScreen->isVisible() && StoryScreen->OnEvent(event))
        return true;
    if (AchievementsScreen && AchievementsScreen->isVisible() && AchievementsScreen->OnEvent(event))
        return true;

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
            case 0:
                setBuildAction(0);
                return true;
            case 1:
                setBuildAction(4);
                return true;
            case 2:
                setBuildAction(1);
                return true;
            case 3:
                setBuildAction(7);
                return true;
            case 4:
                setBuildAction(8);
                return true;
            case GUI_ID_PRIORITIES:
                if (GameMode != game::EGM_CAMPAIGN)
                {
                    m_2c0 = GameSpeed;
                    GameSpeed = 0;
                    PriorityScreen->setVisible(true, ThreatLevel);
                }
                else
                    GUIEnvironment->addMessageBox(L"Attack Priorities",
                        L"Attack priorities are not available during the introduction.", true, 1, 0, -1);
                return true;
            case GUI_ID_SPEED_PAUSE:
                setGameSpeed(0);
                return true;
            case GUI_ID_SPEED_SLOW:
                setGameSpeed(1);
                return true;
            case GUI_ID_SPEED_NORMAL:
                setGameSpeed(3);
                return true;
            case GUI_ID_SPEED_DOUBLE:
                setGameSpeed(5);
                return true;
            case GUI_ID_SPEED_FOUR:
                setGameSpeed(6);
                return true;
            case GUI_ID_BUILDINGS_LEFT:
            {
                ox::core::CRect<int> area = BuildingsArea->getAbsolutePosition();
                int position = BuildingsList->getRelativePosition().UpperLeftCorner.X;
                if (position < 0)
                {
                    BuildingsScrollTarget = (float)(position + area.LowerRightCorner.X - 60 - area.UpperLeftCorner.X);
                    if (BuildingsScrollTarget > 0.0f)
                        BuildingsScrollTarget = 0.0f;
                    BuildingsScrollPosition = (float)position;
                    BuildingsScrolling = true;
                }
                return false;
            }
            case GUI_ID_BUILDINGS_RIGHT:
            {
                ox::core::CRect<int> area = BuildingsArea->getAbsolutePosition();
                ox::core::CRect<int> list = BuildingsList->getRelativePosition();
                int width = area.LowerRightCorner.X - area.UpperLeftCorner.X;
                if (list.LowerRightCorner.X > width)
                {
                    int listWidth = list.LowerRightCorner.X - list.UpperLeftCorner.X;
                    float target = (float)(60 - width + list.UpperLeftCorner.X);
                    if ((int)target + listWidth < width)
                        target = (float)(width - listWidth);
                    BuildingsScrollTarget = target;
                    BuildingsScrollPosition = (float)list.UpperLeftCorner.X;
                    BuildingsScrolling = true;
                }
                return false;
            }
            case GUI_ID_MENU:
                if (IngameMenuScreen)
                {
                    m_2c0 = GameSpeed;
                    GameSpeed = 0;
                    IngameMenuScreen->setVisible(true, GameTime > 600.0f, GameMode);
                }
                return true;
            case GUI_ID_DESELECT:
                setNoneAction(true);
                return true;
            case GUI_ID_UNLINK:
                if (SelectedEntity)
                    SelectedEntity->handleDoubleClickSelection();
                return true;
            case GUI_ID_OVERCHARGE:
                if (SelectedEntity && SelectedEntity->getEntityType() == 1)
                    ((entity::CSparkMoverEntity*)SelectedEntity)->startCharging();
                return true;
            case GUI_ID_EAGLE:
                if (SelectedEntity && SelectedEntity->getEntityType() == 8)
                    replaceSelectedMissileTurret(13);
                return true;
            case GUI_ID_TEMPEST:
                if (SelectedEntity && SelectedEntity->getEntityType() == 8)
                    replaceSelectedMissileTurret(14);
                return true;
            case GUI_ID_DEATHSTAR:
                makeSelectedDeathstarTower();
                return true;
            case GUI_ID_UNLINK_DEATHSTAR:
                unmakeSelectedDeathstarTower();
                return true;
            case GUI_ID_END_LASER:
                if (SelectedEntity && SelectedEntity->getEntityType() == 7)
                    ((entity::CDefenseTowerEntity*)SelectedEntity)->makeEndLaser(0);
                return true;
            case GUI_ID_SPEED_BUILD:
                SelectedEntity->handleDoubleClickSelection();
                return true;
            case GUI_ID_UNLINK_SPEED_BUILD:
                SelectedEntity->handleDoubleClickSelection();
                return true;
            case GUI_ID_REPLACE_PRODUCER:
                replaceSelectedProducer(0);
                return true;
            case GUI_ID_SELL_HARVESTERS:
                sellAllHarvesters();
                return true;
            case GUI_ID_FIRST_LUA_ACTION: case GUI_ID_FIRST_LUA_ACTION + 1: case GUI_ID_FIRST_LUA_ACTION + 2:
            case GUI_ID_FIRST_LUA_ACTION + 3: case GUI_ID_FIRST_LUA_ACTION + 4: case GUI_ID_FIRST_LUA_ACTION + 5:
            case GUI_ID_FIRST_LUA_ACTION + 6: case GUI_ID_FIRST_LUA_ACTION + 7: case GUI_ID_FIRST_LUA_ACTION + 8:
                // The upgrade buttons that scripts add to the buildings.
                if (SelectedEntity && SelectedEntity->getEntityType() != 5 && LuaManager)
                {
                    game::SLuaEntityActionButton* button = LuaManager->getEntityActionButton(
                        id - GUI_ID_FIRST_LUA_ACTION, ((entity::CBuildingEntity*)SelectedEntity)->getBuildingType());
                    if (button->Upgrade)
                        replaceBuildingWithType(SelectedEntity, button->Command.c_str());
                }
                return true;
            case GUI_ID_WAVE_SEND:
                if (GameMode == game::EGM_CREATIVE || GameMode == game::EGM_WAVE)
                    toggleWaveList();
                return true;
            case GUI_ID_CREATIVE_PLACE:
                if (GameMode == game::EGM_CREATIVE)
                    toggleCreativeList();
                return true;
            case GUI_ID_FIRST_WAVE: case GUI_ID_FIRST_WAVE + 1: case GUI_ID_FIRST_WAVE + 2: case GUI_ID_FIRST_WAVE + 3:
            case GUI_ID_FIRST_WAVE + 4: case GUI_ID_FIRST_WAVE + 5: case GUI_ID_FIRST_WAVE + 6:
            case GUI_ID_FIRST_WAVE + 7: case GUI_ID_FIRST_WAVE + 8: case GUI_ID_FIRST_WAVE + 9:
                if (GameMode == game::EGM_WAVE)
                    launchWaveLevel(id - GUI_ID_FIRST_WAVE);
                else if (GameMode == game::EGM_CREATIVE && LuaManager)
                    LuaManager->hookWaveButton(id - GUI_ID_FIRST_WAVE);
                return true;
            case GUI_ID_FIRST_CREATIVE_ALIEN: case GUI_ID_FIRST_CREATIVE_ALIEN + 1:
            case GUI_ID_FIRST_CREATIVE_ALIEN + 2: case GUI_ID_FIRST_CREATIVE_ALIEN + 3:
            case GUI_ID_FIRST_CREATIVE_ALIEN + 4: case GUI_ID_FIRST_CREATIVE_ALIEN + 5:
            case GUI_ID_FIRST_CREATIVE_ALIEN + 6: case GUI_ID_FIRST_CREATIVE_ALIEN + 7:
            case GUI_ID_FIRST_CREATIVE_ALIEN + 8: case GUI_ID_FIRST_CREATIVE_ALIEN + 9:
                if (GameMode == game::EGM_CREATIVE)
                    setPlaceAlienAction(id - GUI_ID_FIRST_CREATIVE_ALIEN);
                return true;
            case GUI_ID_BUY:
                Device->getOSOperator()->openURL(L"http://www.oxeyegames.com/harvest/buy-now");
                NextState = EGS_QUIT;
                return true;
            case GUI_ID_BUY_CLOSE:
                if (BuyMessageBox)
                {
                    BuyMessageBox->remove();
                    BuyMessageBox = 0;
                    if (m_2c0)
                        togglePause();
                }
                return true;
            default:
                return false;
            }
        case ox::gui::EGET_CHECKBOX_TOGGLED:
            if (id != GUI_ID_RECYCLE)
                return false;
            if (RecycleButton->isChecked())
                setRecycleAction();
            else
                setNoneAction(true);
            return false;
        case ox::gui::EGET_EDITBOX_ENTER:
            if (id != GUI_ID_SCRIPT_INPUT)
                return false;
            // The script text input was entered.
            m_7b0->setVisible(false);
            m_7b0->setEnabled(false);
            if (LuaManager)
                LuaManager->hookTextInput(m_7b0->getText());
            return true;
        case ox::gui::EGET_ELEMENT_DRAWN:
            switch (id)
            {
            case GUI_ID_FIRST_WAVE: case GUI_ID_FIRST_WAVE + 1: case GUI_ID_FIRST_WAVE + 2: case GUI_ID_FIRST_WAVE + 3:
            case GUI_ID_FIRST_WAVE + 4: case GUI_ID_FIRST_WAVE + 5: case GUI_ID_FIRST_WAVE + 6:
            case GUI_ID_FIRST_WAVE + 7: case GUI_ID_FIRST_WAVE + 8: case GUI_ID_FIRST_WAVE + 9:
            {
                int wave = id - GUI_ID_FIRST_WAVE;
                ox::core::CString<wchar_t> label(ThreatLevel->getThreatLevel() + 1);
                bool aliens[WAVE_COUNT];
                int count;
                int first;
                if (GameMode == game::EGM_WAVE)
                {
                    first = -1;
                    count = 0;
                    for (int i = 0; i < WAVE_COUNT; ++i)
                    {
                        aliens[i] = ThreatLevel->alienIsPresentOnThisWave(game::gp_world->getPlanet(), wave, i);
                        if (aliens[i])
                        {
                            ++count;
                            if (first < 0)
                                first = i;
                        }
                    }
                }
                else if (LuaManager)
                {
                    label = ox::core::CString<wchar_t>(LuaManager->getWaveButtonNumber(wave));
                    first = -1;
                    count = 0;
                    for (int i = 0; i < WAVE_COUNT; ++i)
                    {
                        aliens[i] = LuaManager->isAlienOnButton(wave, i);
                        if (aliens[i])
                        {
                            ++count;
                            if (first < 0)
                                first = i;
                        }
                    }
                }
                else
                {
                    first = -1;
                    count = 0;
                }
                renderWaveButton(event.GUIEvent.Caller, count, first, aliens, label);
                return true;
            }
            case GUI_ID_FIRST_CREATIVE_ALIEN: case GUI_ID_FIRST_CREATIVE_ALIEN + 1:
            case GUI_ID_FIRST_CREATIVE_ALIEN + 2: case GUI_ID_FIRST_CREATIVE_ALIEN + 3:
            case GUI_ID_FIRST_CREATIVE_ALIEN + 4: case GUI_ID_FIRST_CREATIVE_ALIEN + 5:
            case GUI_ID_FIRST_CREATIVE_ALIEN + 6: case GUI_ID_FIRST_CREATIVE_ALIEN + 7:
            case GUI_ID_FIRST_CREATIVE_ALIEN + 8: case GUI_ID_FIRST_CREATIVE_ALIEN + 9:
            {
                int alien = id - GUI_ID_FIRST_CREATIVE_ALIEN;
                ox::core::CString<wchar_t> label(L"");
                bool aliens[WAVE_COUNT];
                for (int i = 0; i < WAVE_COUNT; ++i)
                    aliens[i] = false;
                aliens[alien] = true;
                renderWaveButton(event.GUIEvent.Caller, 1, alien, aliens, label);
                return true;
            }
            case GUI_ID_BOTTOM_BAR:
                if (BottomBar)
                {
                    GuiSprites[GS_BOTTOM_LEFT_BACKGROUND]->draw(BarLeftArea.UpperLeftCorner, 0,
                        ox::video::SColor(0xffffffff));
                    GuiSprites[GS_BOTTOM_RIGHT_BACKGROUND]->draw(BarRightArea.UpperLeftCorner, 0,
                        ox::video::SColor(0xffffffff));
                    ox::core::CDimension2d<int> size = GuiSprites[GS_BOTTOM_CENTER_BACKGROUND]->getFrameSize(0);
                    for (int x = BarCenterArea.UpperLeftCorner.X; x < BarCenterArea.LowerRightCorner.X;
                         x += size.Width)
                        GuiSprites[GS_BOTTOM_CENTER_BACKGROUND]->draw(
                            ox::core::CPosition2d<int>(x, BarCenterArea.UpperLeftCorner.Y), &BarCenterArea,
                            ox::video::SColor(0xffffffff));
                }
                return true;
            case GUI_ID_ACTION_PANEL:
                // The action panel shows the selected building.
                if (ActionPanel)
                {
                    ox::core::CRect<int> panel = ActionPanel->getAbsolutePosition();
                    GuiSprites[GS_TOP_LEFT_BACKGROUND]->draw(panel.UpperLeftCorner, 0, ox::video::SColor(0xffffffff));
                    if (SelectedEntity && SelectedEntity->getCurrentDisplaySprite())
                        SelectedEntity->getCurrentDisplaySprite()->drawScaled(
                            ox::core::CPosition2d<float>(panel.UpperLeftCorner.X + 56.0f,
                                panel.UpperLeftCorner.Y + 55.0f),
                            0.8f, ox::video::SColor(0xffffffff));
                }
                return true;
            case GUI_ID_TOP_BAR:
                if (TopBar)
                    renderMinimap();
                return true;
            case GUI_ID_ENERGY_POPUP:
                if (GuiSprites[GS_ICON_ENERGY])
                {
                    ox::core::CRect<int> popup = event.GUIEvent.Caller->getAbsolutePosition();
                    GuiSprites[GS_ICON_ENERGY]->draw(
                        ox::core::CPosition2d<int>(popup.UpperLeftCorner.X + 11, popup.UpperLeftCorner.Y + 11), 0,
                        ox::video::SColor(0xffffffff));
                }
                return true;
            case GUI_ID_MINERALS_POPUP:
                if (GuiSprites[GS_ICON_CREDITS])
                {
                    ox::core::CRect<int> popup = event.GUIEvent.Caller->getAbsolutePosition();
                    GuiSprites[GS_ICON_CREDITS]->draw(
                        ox::core::CPosition2d<int>(popup.UpperLeftCorner.X + 12, popup.UpperLeftCorner.Y + 11), 0,
                        ox::video::SColor(0xffffffff));
                }
                return true;
            case GUI_ID_BUILDINGS_LIST:
            {
                // The build buttons.
                ox::core::CRect<int> clip = BuildingsArea->getAbsoluteClippingRect();
                ox::core::CPosition2d<int> mouse = GUIEnvironment->getMousePosition();
                int selected = -1;
                if (Action == 1)
                    selected = BuildSelection;
                BuildableItems.renderButtonLayouts(mouse, selected, clip);
                return true;
            }
            default:
                return false;
            }
        default:
            return false;
        }
    }
    case ox::event::EET_MOUSE_INPUT_EVENT:
    {
        ox::core::CPosition2d<int> mouse(event.MouseInput.X, event.MouseInput.Y);
        ox::core::CPosition2d<float> world = getWorldPos(mouse);
        int x = mouse.X;
        int y = mouse.Y;
        switch (event.MouseInput.Event)
        {
        case ox::event::EMIE_LMOUSE_PRESSED_DOWN:
            if (!MinimapRect.isPointInside(mouse))
            {
                if (BottomBar->getAbsolutePosition().isPointInside(mouse))
                {
                    if (BuildingsArea->getAbsolutePosition().isPointInside(mouse))
                    {
                        for (int i = 0; i < BuildableItems.getNumBuildings(); ++i)
                        {
                            if (BuildableItems.isPointWithinButton(i, ox::core::CPosition2d<int>(x, y)))
                            {
                                setBuildAction(i);
                                return true;
                            }
                        }
                    }
                    return true;
                }
                if (GameOver)
                {
                    // A click skips the end of the game.
                    GameOverTime = 0.1f;
                }
                else if (Action == 0)
                {
                    LastPlacement = world;
                    HasLastPlacement = false;
                    entity::CEntity* clicked = entity::gp_entityManager->findClickableEntity(world);
                    if (!clicked)
                    {
                        // Starts a rectangle selection.
                        SelectionClickPending = true;
                        if (LuaManager)
                            LuaManager->hookMouseClick(world, 0, true);
                        return false;
                    }
                    if (m_keys[ox::KEY_CONTROL])
                    {
                        // Selects the buildings of the type on the screen.
                        int type = clicked->getEntityType();
                        ox::core::CDimension2d<int> view = getViewSize();
                        ox::core::CPosition2d<float> corner1 = ViewPosition;
                        ox::core::CPosition2d<float> corner2(view.Width + ViewPosition.X, view.Height + ViewPosition.Y);
                        performRectangleSelection(corner1, corner2,
                            m_keys[ox::KEY_SHIFT] || m_keys[ox::KEY_LSHIFT] || m_keys[ox::KEY_RSHIFT], type);
                        return true;
                    }
                    if (m_keys[ox::KEY_SHIFT] || m_keys[ox::KEY_LSHIFT] || m_keys[ox::KEY_RSHIFT])
                    {
                        buildingShiftSelected(clicked);
                        return true;
                    }
                    if (clicked == SelectedEntity && event.MouseInput.Clicks > 1)
                        clicked->handleDoubleClickSelection();
                    if (SelectedEntityId != clicked->getId() && AudioDriver)
                        AudioDriver->playSound("SelectBuilding.ogg", 1.0f, 0.0f, 1.0f);
                    if (LuaManager)
                        LuaManager->hookUnitSelected(clicked->getId());
                    SelectedEntity = clicked;
                    SelectedEntityId = clicked->getId();
                    DraggingFromSelection = true;
                    newSelectedEntity();
                    return true;
                }
                else if (Action == 1)
                {
                    buyBuildingAtPlacementPos();
                    return true;
                }
                else if (Action == 2)
                {
                    if (RecycleTarget)
                    {
                        sellEntity(RecycleTarget);
                        RecycleTarget = 0;
                        CursorMoved = true;
                    }
                }
                else if (Action == 3)
                {
                    if ((unsigned int)AlienSelection < WAVE_COUNT)
                    {
                        placeCurrentAlienSelection(world);
                        LastPlacement = world;
                        HasLastPlacement = true;
                    }
                }
                return false;
            }
            else
            {
                // Clicks on the minimap move the view there.
                MinimapDragging = true;
                ox::core::CRect<float> field = game::gp_world->getVisibleGameFieldSize();
                ViewPosition.X = ox::core::clamp((float)(x - MinimapRect.UpperLeftCorner.X) *
                    ((field.LowerRightCorner.X - field.UpperLeftCorner.X) /
                    (float)(MinimapRect.LowerRightCorner.X - MinimapRect.UpperLeftCorner.X)) +
                    ScreenSizeF.Width * -0.5f + field.UpperLeftCorner.X,
                    field.UpperLeftCorner.X, field.LowerRightCorner.X - ScreenSizeF.Width);
                ViewPosition.Y = ox::core::clamp((float)(y - MinimapRect.UpperLeftCorner.Y) *
                    ((field.LowerRightCorner.Y - field.UpperLeftCorner.Y) /
                    (float)(MinimapRect.LowerRightCorner.Y - MinimapRect.UpperLeftCorner.Y)) +
                    ScreenSizeF.Height * -0.5f + field.UpperLeftCorner.Y,
                    field.UpperLeftCorner.Y, field.LowerRightCorner.Y - ScreenSizeF.Height);
                return true;
            }
        case ox::event::EMIE_RMOUSE_PRESSED_DOWN:
            if (Action == 0)
            {
                if (SelectedEntity)
                    SelectedEntity->handleRightClickAction(world);
                else if (LuaManager)
                    LuaManager->hookMouseClick(world, 1, true);
            }
            else if (Action >= 1 && Action <= 3)
                setNoneAction(true);
            return false;
        case ox::event::EMIE_MMOUSE_PRESSED_DOWN:
        {
            // The middle button scrolls the view.
            MiddleMouseScrolling = true;
            Device->getCursorControl()->setVisible(false);
            ox::gui::ICursorControl* cursor = Device->getCursorControl();
            cursor->setPosition(Device->getCursorControl()->getPosition());
            if (LuaManager)
                LuaManager->hookMouseClick(world, 2, true);
            return false;
        }
        case ox::event::EMIE_LMOUSE_LEFT_UP:
            if (RectangleSelecting)
                performRectangleSelection(LastPlacement, world,
                    m_keys[ox::KEY_SHIFT] || m_keys[ox::KEY_LSHIFT] || m_keys[ox::KEY_RSHIFT], -1);
            MinimapDragging = false;
            DraggingFromSelection = false;
            HasLastPlacement = false;
            SelectionClickPending = false;
            RectangleSelecting = false;
            if (LuaManager)
                LuaManager->hookMouseClick(world, 0, false);
            return false;
        case ox::event::EMIE_RMOUSE_LEFT_UP:
            if (LuaManager)
                LuaManager->hookMouseClick(world, 1, false);
            return false;
        case ox::event::EMIE_MMOUSE_LEFT_UP:
            if (MiddleMouseScrolling)
            {
                MiddleMouseScrolling = false;
                Device->getCursorControl()->setVisible(true);
                ox::gui::ICursorControl* cursor = Device->getCursorControl();
                cursor->setPosition(Device->getCursorControl()->getPosition());
            }
            if (LuaManager)
                LuaManager->hookMouseClick(world, 2, false);
            return false;
        case ox::event::EMIE_MOUSE_MOVED:
            if (MinimapDragging)
            {
                ox::core::CRect<float> field = game::gp_world->getVisibleGameFieldSize();
                float scaleX = (field.LowerRightCorner.X - field.UpperLeftCorner.X) /
                    (float)(MinimapRect.LowerRightCorner.X - MinimapRect.UpperLeftCorner.X);
                float scaleY = (field.LowerRightCorner.Y - field.UpperLeftCorner.Y) /
                    (float)(MinimapRect.LowerRightCorner.Y - MinimapRect.UpperLeftCorner.Y);
                ox::core::CDimension2d<int> view = getViewSize();
                ViewPosition.X = (float)(x - MinimapRect.UpperLeftCorner.X) * scaleX + (float)view.Width * -0.5f +
                    field.UpperLeftCorner.X;
                ViewPosition.Y = (float)(y - MinimapRect.UpperLeftCorner.Y) * scaleY + (float)view.Height * -0.5f +
                    field.UpperLeftCorner.Y;
                game::gp_world->constrainViewPos(ViewPosition);
            }
            else if (DraggingFromSelection && SelectedEntity)
            {
                if (!HasLastPlacement)
                {
                    if (ox::core::CMath::getEstimateDistance(world, LastPlacement) > 10.0f)
                        HasLastPlacement = true;
                }
                else
                {
                    // Dragging from the selection to another building links them.
                    entity::CEntity* target = entity::gp_entityManager->findClickableEntity(world);
                    if (!target)
                        SelectedEntity->handleSelectionDraggedToNothing();
                    else if (SelectedEntity->handleSelectionDraggedToEntity(target))
                    {
                        SelectedEntity = target;
                        SelectedEntityId = target->getId();
                        DraggingFromSelection = true;
                        HasLastPlacement = false;
                        LastPlacement = world;
                        newSelectedEntity();
                        if (AudioDriver)
                            AudioDriver->playSound("SelectBuilding.ogg", 1.0f, 0.0f, 1.0f);
                        if (LuaManager)
                            LuaManager->hookUnitSelected(target->getId());
                        return true;
                    }
                }
            }
            else if (Action == 3 && HasLastPlacement)
            {
                // Dragging places more aliens.
                if (ox::core::CMath::getEstimateDistance(world, LastPlacement) > 10.0f)
                {
                    LastPlacement = world;
                    if ((unsigned int)AlienSelection < WAVE_COUNT)
                        placeCurrentAlienSelection(world);
                }
            }
            else
            {
                CursorMoved = true;
                if ((m_keys[ox::KEY_SPACE] || MiddleMouseScrolling) && game::gp_world)
                {
                    // Scrolls the view by the mouse movement and keeps the cursor in place.
                    if (MousePosition.X != x || MousePosition.Y != y)
                    {
                        ViewPosition.X = ViewPosition.X + (float)(x - MousePosition.X);
                        ViewPosition.Y = ViewPosition.Y + (float)(y - MousePosition.Y);
                        game::gp_world->constrainViewPos(ViewPosition);
                        Device->getCursorControl()->setVisible(false);
                        Device->getCursorControl()->setPosition(MousePosition);
                    }
                }
                else
                {
                    MousePosition = ox::core::CPosition2d<int>(x, y);
                    if (GameMode == game::EGM_WAVE || GameMode == game::EGM_CREATIVE)
                    {
                        // The lists close when the mouse leaves them.
                        for (int i = 0; i < LIST_COUNT; ++i)
                        {
                            if (!ListVisible[i] || !ListGroups[i])
                                continue;
                            ox::core::CRect<int> area = ListGroups[i]->getAbsolutePosition();
                            area.UpperLeftCorner.X -= 100;
                            area.UpperLeftCorner.Y -= 50;
                            area.LowerRightCorner.Y += 50;
                            if (!area.isPointInside(mouse))
                            {
                                if (i == LIST_WAVES)
                                    setWaveListToggle(false);
                                else
                                    setCreativeListToggle(false);
                            }
                        }
                    }
                }
            }
            if (SelectionClickPending && ox::core::CMath::getEstimateDistance(world, LastPlacement) > 10.0f)
                RectangleSelecting = true;
            return false;
        case ox::event::EMIE_MOUSE_WHEEL:
            ViewPosition -= ox::core::CPosition2d<float>((int)(ScrollSpeed * event.MouseInput.ScrollX),
                (int)(event.MouseInput.ScrollY * ScrollSpeed));
            game::gp_world->constrainViewPos(ViewPosition);
            Device->getCursorControl()->setPosition(MousePosition);
            return true;
        default:
            return false;
        }
    }
    case ox::event::EET_KEY_INPUT_EVENT:
        switch (event.KeyInput.Event)
        {
        case ox::event::EKIE_KEY_PRESSED_DOWN:
            m_keys[event.KeyInput.Key] = true;
            if (event.KeyInput.Key == ox::KEY_RETURN && m_7b0)
            {
                // Opens the script text input.
                m_7b0->setText(L"");
                m_7b0->setEnabled(true);
                m_7b0->setVisible(true);
                GUIEnvironment->setFocus(m_7b0);
                return true;
            }
            switch (Profile->getCommandForKey(event.KeyInput.Key))
            {
            case settings::EKC_INCREASE_SPEED:
                if (GameSpeed <= 5)
                    setGameSpeed(GameSpeed + 1);
                return true;
            case settings::EKC_DECREASE_SPEED:
                if (GameSpeed > 0)
                    setGameSpeed(GameSpeed - 1);
                return true;
            case settings::EKC_SPEED_PAUSED:
                setGameSpeed(0);
                return true;
            case settings::EKC_SPEED_SLOWER:
                setGameSpeed(1);
                return true;
            case settings::EKC_SPEED_NORMAL:
                setGameSpeed(3);
                return true;
            case settings::EKC_SPEED_FASTER:
                setGameSpeed(5);
                return true;
            case settings::EKC_SPEED_FASTEST:
                setGameSpeed(6);
                return true;
            case settings::EKC_SPEED_PAUSE_TOGGLE:
                togglePause();
                return true;
            case settings::EKC_BUILD_PRODUCER:
                setBuildAction(BuildableItems.getIndexForEntityType(0));
                return true;
            case settings::EKC_BUILD_MOVER:
                setBuildAction(BuildableItems.getIndexForEntityType(1));
                return true;
            case settings::EKC_BUILD_MINER:
                setBuildAction(BuildableItems.getIndexForEntityType(4));
                return true;
            case settings::EKC_BUILD_TOWER:
                setBuildAction(BuildableItems.getIndexForEntityType(7));
                return true;
            case settings::EKC_BUILD_LAUNCHER:
                setBuildAction(BuildableItems.getIndexForEntityType(8));
                return true;
            case settings::EKC_ACTION_SPECIAL:
                if (SelectedEntity)
                {
                    switch (SelectedEntity->getEntityType())
                    {
                    case 0:
                        if (((entity::CSparkProducerEntity*)SelectedEntity)->isExpired())
                            replaceSelectedProducer(0);
                        break;
                    case 1:
                        ((entity::CSparkMoverEntity*)SelectedEntity)->startCharging();
                        break;
                    case 4:
                        sellAllHarvesters();
                        break;
                    case 7:
                        if (((entity::CDefenseTowerEntity*)SelectedEntity)->getNumBackTargets() == 0)
                            makeSelectedDeathstarTower();
                        else if (((entity::CDefenseTowerEntity*)SelectedEntity)->getNumBackTargets() != 0)
                            unmakeSelectedDeathstarTower();
                        break;
                    }
                }
                return true;
            case settings::EKC_ACTION_EAGLE:
                if (SelectedEntity && SelectedEntity->getEntityType() == 8 && !SelectedEntity->isKilled())
                    replaceSelectedMissileTurret(13);
                return true;
            case settings::EKC_ACTION_TEMPEST:
                if (SelectedEntity && SelectedEntity->getEntityType() == 8 && !SelectedEntity->isKilled())
                    replaceSelectedMissileTurret(14);
                return true;
            case settings::EKC_ACTION_SELL:
                if (SelectedEntity && SelectedEntity->getEntityType() != 5)
                {
                    sellEntity(SelectedEntity);
                    setNoneAction(true);
                }
                else if (MultiSelection.begin() != MultiSelection.end())
                {
                    for (ox::TArray<ox::entity::SEntityReference*>::iterator it = MultiSelection.begin();
                         it != MultiSelection.end(); ++it)
                        if (((entity::CEntity*)(*it)->Entity)->getEntityType() != 5)
                            sellEntity((entity::CEntity*)(*it)->Entity);
                    setNoneAction(true);
                }
                return true;
            case settings::EKC_ACTION_SOMETHING:
                if (SelectedEntity && SelectedEntity->getEntityType() == 7)
                {
                    ((entity::CDefenseTowerEntity*)SelectedEntity)->makeEndLaser(0);
                    return true;
                }
                if (SelectedEntity && SelectedEntity->getEntityType() == 3)
                {
                    SelectedEntity->handleDoubleClickSelection();
                    return true;
                }
                return false;
            case settings::EKC_GAME_SETTINGS:
                m_2c0 = GameSpeed;
                GameSpeed = 0;
                SettingsScreen->setVisible(true);
                return true;
            case settings::EKC_GAME_PRIORITIES:
                m_2c0 = GameSpeed;
                GameSpeed = 0;
                PriorityScreen->setVisible(true, ThreatLevel);
                return true;
            case settings::EKC_GAME_RANGES:
                ShowAllRanges = true;
                return true;
            case settings::EKC_GAME_OVERHEATS:
                entity::g_useLargeSparkDeathParticle = true;
                return true;
            default:
                break;
            }
            switch (event.KeyInput.Key)
            {
            case ox::KEY_ESCAPE:
                if (m_7b0 && m_7b0->isVisible())
                {
                    m_7b0->setVisible(false);
                    m_7b0->setEnabled(false);
                }
                else if (Action != 0 || SelectedEntity || MultiSelection.begin() != MultiSelection.end())
                    setNoneAction(true);
                else if (IngameMenuScreen)
                {
                    m_2c0 = GameSpeed;
                    GameSpeed = 0;
                    IngameMenuScreen->setVisible(true, GameTime > 600.0f, GameMode);
                }
                return true;
            case ox::KEY_KEY_F:
                if (event.KeyInput.Control || event.KeyInput.Shift)
                    ShowDebugInfo = !ShowDebugInfo;
                return true;
            case ox::KEY_KEY_Y:
                if (event.KeyInput.Control || event.KeyInput.Shift)
                    ShowMouseWorldPos = !ShowMouseWorldPos;
                return true;
            case ox::KEY_F5:
                sendCustomEvent(ECE_QUICK_SAVE);
                return true;
            case ox::KEY_F7:
                sendCustomEvent(ECE_QUICK_LOAD);
                return true;
            default:
                return false;
            }
        case ox::event::EKIE_KEY_LEFT_UP:
            m_keys[event.KeyInput.Key] = false;
            switch (Profile->getCommandForKey(event.KeyInput.Key))
            {
            case settings::EKC_GAME_RANGES:
                ShowAllRanges = false;
                break;
            case settings::EKC_GAME_OVERHEATS:
                entity::g_useLargeSparkDeathParticle = false;
                break;
            default:
                break;
            }
            if (event.KeyInput.Key == ox::KEY_SPACE)
            {
                Device->getCursorControl()->setVisible(true);
                ox::gui::ICursorControl* cursor = Device->getCursorControl();
                cursor->setPosition(Device->getCursorControl()->getPosition());
            }
            return false;
        default:
            return false;
        }
    case ox::event::EET_JOYSTICK_INPUT_EVENT:
    {
        // Joystick buttons click at the screen center or change the building to build.
        ox::event::SEvent mouse;
        mouse.EventType = ox::event::EET_MOUSE_INPUT_EVENT;
        mouse.MouseInput.X = ScreenSize.Width / 2;
        mouse.MouseInput.Y = ScreenSize.Height / 2;
        mouse.MouseInput.Clicks = 1;
        switch (event.JoystickEvent.Type)
        {
        case 0:
            if (event.JoystickEvent.Button == 0)
            {
                mouse.MouseInput.Event = ox::event::EMIE_LMOUSE_PRESSED_DOWN;
                OnEvent(mouse);
            }
            else if (event.JoystickEvent.Button == 1)
            {
                mouse.MouseInput.Event = ox::event::EMIE_RMOUSE_PRESSED_DOWN;
                OnEvent(mouse);
            }
            else if (event.JoystickEvent.Button == 2)
            {
                if (Action == 1)
                    changeConstructionSelection(false);
                else
                    setBuildAction(BuildSelection);
            }
            else if (event.JoystickEvent.Button == 3)
            {
                if (Action == 1)
                    changeConstructionSelection(true);
                else
                    setBuildAction(BuildSelection);
            }
            return true;
        case 1:
            if (event.JoystickEvent.Button == 0)
            {
                mouse.MouseInput.Event = ox::event::EMIE_LMOUSE_LEFT_UP;
                OnEvent(mouse);
            }
            else if (event.JoystickEvent.Button == 1)
            {
                mouse.MouseInput.Event = ox::event::EMIE_RMOUSE_LEFT_UP;
                OnEvent(mouse);
            }
            return true;
        default:
            return false;
        }
    }
    case ox::event::EET_DEVICE_EVENT:
        if (event.DeviceEvent.Type == ox::event::EDE_FULLSCREEN_TOGGLED)
            realignGui();
        return false;
    case ox::event::EET_USER_EVENT:
        switch (event.UserEvent.UserData1)
        {
        case ECE_VIDEO_MODE_CHANGED:
            realignGui();
            return true;
        case ECE_PARTICLE_SETTING_CHANGED:
            ParticleSetting = settings::gp_systemConfig->getParticleSetting();
            return true;
        case ECE_SCROLL_SPEED_CHANGED:
            ScrollSpeed = settings::gp_systemConfig->getScrollSpeed();
            return true;
        case ECE_CONTINUE_GAME:
            if (m_2c0)
                togglePause();
            return true;
        case ECE_SAVE_GAME:
            if (SaveGameScreen)
            {
                if (writeStateToFile(SaveGameScreen->getSelectedSaveFilename(),
                        SaveGameScreen->getSelectedSaveDescription()))
                    addInfoLine(ox::core::CString<wchar_t>(L"Game saved"));
                else
                    addInfoLine(ox::core::CString<wchar_t>(L"Write error! Unable to save game!"));
            }
            return true;
        case ECE_PROFILE_CREATED:
            return false;
        case ECE_BUILDING_ATTACKED:
        {
            BuildingAttacked = true;
            SMinimapMarker marker;
            marker.X = (float)event.UserEvent.UserData2;
            marker.Y = (float)event.UserEvent.UserData3;
            marker.Time = 5.0f;
            MinimapMarkers.push_back(marker);
            return true;
        }
        case ECE_MINERALS_APPEARED:
            if (entity::gp_entityManager)
            {
                const std::list<ox::entity::COxEntity*>& entities = entity::gp_entityManager->getEntityList(0);
                for (std::list<ox::entity::COxEntity*>::const_iterator it = entities.begin(); it != entities.end();
                     ++it)
                    if ((*it)->getEntityType() == 4)
                        ((entity::CMineralGatherEntity*)*it)->notifyMineralsAppeared();
            }
            return true;
        case ECE_REPLACE_BUILDING:
            if (entity::gp_entityManager && entity::gp_buildableItems)
            {
                entity::CEntity* building;
                if (event.UserEvent.UserData2 > 0)
                    building = (entity::CEntity*)entity::gp_entityManager->locateEntity(event.UserEvent.UserData2, 0);
                else
                    building = SelectedEntity;
                if (building && building->getEntityType() != 5)
                    replaceBuildingWithType(building, (const char*)event.UserEvent.UserPointer);
            }
            return true;
        case ECE_NEW_GAME:
            NextState = EGS_PLAY;
            g_gameMode = GameMode;
            g_gamePlanet = game::gp_world->getPlanet();
            g_loadGameFilename = "";
            return true;
        case ECE_START_GAME:
            NextState = EGS_PLAY;
            return true;
        case ECE_LANGUAGE_CHANGED:
            return false;
        case ECE_EXIT_GAME:
            NextState = EGS_MAIN_MENU;
            return true;
        case ECE_SHOW_SAVE_SCREEN:
            if (SaveGameScreen)
                SaveGameScreen->setVisible(true, false);
            return true;
        case ECE_SHOW_LOAD_SCREEN:
            if (SaveGameScreen)
                SaveGameScreen->setVisible(true, true);
            return true;
        case ECE_SHOW_SETTINGS_SCREEN:
            SettingsScreen->setVisible(true);
            return true;
        case ECE_SHOW_AWARDS_SCREEN:
            if (AchievementsScreen)
                AchievementsScreen->setVisible(true);
            return true;
        case ECE_MAIN_ACHIEVEMENT:
            // Achievements count only for the player of the game, and never in creative games.
            if (PlayerName == settings::gp_profileManager->getCurrentProfile()->getPlayerName() &&
                GameMode != game::EGM_CREATIVE)
            {
                int score = settings::gp_profileManager->getCurrentProfile()->getAchievementScore();
                int rating = settings::gp_profileManager->getCurrentProfile()->getAchievementRating(score);
                if (settings::gp_profileManager->getCurrentProfile()->notifyMainAchievement(event.UserEvent.UserData2,
                        game::gp_world->getPlanet()))
                {
                    game::SInfoLineMessage message;
                    message.Name = settings::gp_systemConfig->getLocalizedText(L"achievementInfo:awardedTitle");
                    ox::core::CString<wchar_t> name = settings::gp_systemConfig->getLocalizedText(
                        settings::ACHIEVEMENT_NAMES[event.UserEvent.UserData2]);
                    int newScore = settings::gp_profileManager->getCurrentProfile()->getAchievementScore();
                    int newRating = settings::gp_profileManager->getCurrentProfile()->getAchievementRating(newScore);
                    ox::core::CString<wchar_t> ratingName =
                        settings::gp_systemConfig->getLocalizedText(settings::ACHIEVEMENT_RATING_NAMES[newRating]);
                    if (rating == newRating)
                        message.Text = settings::gp_systemConfig->getLocalizedText(
                            L"achievementInfo:awardedNoChange", name.c_str(), ratingName.c_str());
                    else
                        message.Text = settings::gp_systemConfig->getLocalizedText(L"achievementInfo:awarded",
                            name.c_str(), ratingName.c_str());
                    message.Sound = "Achievement.ogg";
                    message.Portrait = settings::CHarvestProfile::getAchievementSpriteName(event.UserEvent.UserData2,
                        game::gp_world->getPlanet());
                    addInfoLine(&message, true);
                    if (game::gp_statistics)
                        game::gp_statistics->addLog(GameTime, 1, event.UserEvent.UserData2);
                }
                if (score == 144)
                    sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 24);
            }
            return true;
        case ECE_TUTORIAL_HINT:
            // Achievements count only for the player of the game, and never in creative games.
            if (PlayerName == settings::gp_profileManager->getCurrentProfile()->getPlayerName() &&
                GameMode != game::EGM_CREATIVE)
            {
                int score = settings::gp_profileManager->getCurrentProfile()->getAchievementScore();
                int rating = settings::gp_profileManager->getCurrentProfile()->getAchievementRating(score);
                if (settings::gp_profileManager->getCurrentProfile()->notifyMiniAchievement(event.UserEvent.UserData2,
                        game::gp_world->getPlanet()))
                {
                    game::SInfoLineMessage message;
                    message.Name = settings::gp_systemConfig->getLocalizedText(L"achievementInfo:awardedTitle");
                    ox::core::CString<wchar_t> name = settings::gp_systemConfig->getLocalizedText(
                        settings::ACHIEVEMENT_NAMES[event.UserEvent.UserData2]);
                    int newScore = settings::gp_profileManager->getCurrentProfile()->getAchievementScore();
                    int newRating = settings::gp_profileManager->getCurrentProfile()->getAchievementRating(newScore);
                    ox::core::CString<wchar_t> ratingName =
                        settings::gp_systemConfig->getLocalizedText(settings::ACHIEVEMENT_RATING_NAMES[newRating]);
                    if (rating == newRating)
                        message.Text = settings::gp_systemConfig->getLocalizedText(
                            L"achievementInfo:awardedNoChange", name.c_str(), ratingName.c_str());
                    else
                        message.Text = settings::gp_systemConfig->getLocalizedText(L"achievementInfo:awarded",
                            name.c_str(), ratingName.c_str());
                    message.Sound = "MiniAchievement.ogg";
                    message.Portrait = settings::CHarvestProfile::getAchievementSpriteName(event.UserEvent.UserData2,
                        game::gp_world->getPlanet());
                    addInfoLine(&message, true);
                    if (game::gp_statistics)
                        game::gp_statistics->addLog(GameTime, 1, event.UserEvent.UserData2);
                }
                if (score == 144)
                    sendCustomEvent(ECE_MAIN_ACHIEVEMENT, 24);
            }
            return true;
        case ECE_BOSS_WARNING:
        {
            game::SInfoLineMessage message;
            message.Name = settings::gp_systemConfig->getLocalizedText(L"ingame:bossWarningTitle");
            message.Portrait = "PortraitCommunications";
            message.Text = settings::gp_systemConfig->getLocalizedText(L"ingame:bossWarningMessage");
            message.Sound = "ingame_infoBoss.ogg";
            addInfoLine(&message, false);
            return true;
        }
        case ECE_END_INITIAL_WORLD:
            game::gp_world->InitialWorld = false;
            return true;
        case ECE_SKIP_STORY:
            if (Scenario)
            {
                HighlightActive = false;
                Scenario->skipToNextEvent();
            }
            return true;
        case ECE_GAME_WON:
            if (!GameOver)
            {
                g_scenarioResult = (GameMode != game::EGM_CAMPAIGN) * 2 + 1;
                g_scenarioResultGameMode = GameMode;
                g_scenarioResultPlanet = game::gp_world->getPlanet();
                GameOver = true;
                GameWon = true;
                GameOverTime = 15.0f;
                if (game::gp_statistics && GameMode == game::EGM_CREATIVE)
                    game::gp_statistics->reportNewThreatLevel((int)GameTime / 300 + 1, GameTime);
            }
            return true;
        case ECE_GAME_LOST:
            if (!GameOver)
            {
                GameOver = true;
                GameWon = false;
                GameOverTime = 15.0f;
                g_scenarioResult = 4;
                if (game::gp_statistics && GameMode == game::EGM_CREATIVE)
                    game::gp_statistics->reportNewThreatLevel((int)GameTime / 300 + 1, GameTime);
            }
            return true;
        case ECE_JUMP_TO_ENTITY:
            FollowJump = true;
            // fall through
        case ECE_FOLLOW_ENTITY:
            Follow.Entity = 0;
            Follow.Id = event.UserEvent.UserData2;
            FollowLayer = event.UserEvent.UserData3;
            return true;
        case ECE_MOVE_VIEW:
            ViewPosition.X = (float)event.UserEvent.UserData2;
            ViewPosition.Y = (float)event.UserEvent.UserData3;
            if (game::gp_world)
                game::gp_world->constrainViewPos(ViewPosition);
            return true;
        case ECE_SHOW_DIALOGUE:
            if (StoryScreen && event.UserEvent.UserPointer)
            {
                // The keys held for the game are released for the dialogue.
                for (int i = 0; i < 256; ++i)
                    m_keys[i] = false;
                StoryScreen->displayDialogueText((gui::CDialogueItemInfo*)event.UserEvent.UserPointer);
            }
            return true;
        case ECE_ADD_INFO_MESSAGE:
            addInfoLine((game::SInfoLineMessage*)event.UserEvent.UserPointer, false);
            return true;
        case ECE_ADD_INFO_TEXT:
            addInfoLine(ox::core::CString<wchar_t>((const wchar_t*)event.UserEvent.UserPointer));
            return true;
        case ECE_CLOSE_DIALOGUE:
            if (StoryScreen)
                StoryScreen->CloseWhenDone = true;
            return true;
        case ECE_DROPSHIP_KILLED_ALL_ALIENS:
            if (Scenario)
                Scenario->notifyDropshipKilledAllAliens(this);
            return true;
        case ECE_DROPSHIP_LANDED:
            if (Scenario)
                Scenario->notifyDropshipLanded(this);
            return true;
        case ECE_DROPSHIP_GOING_TO_SPACE:
            if (Scenario)
                Scenario->notifyDropshipGoingToSpace(this);
            return true;
        case ECE_HIGHLIGHT_ENTITIES:
            HighlightActive = true;
            HighlightTime = 10.0f;
            HighlightEntityType = event.UserEvent.UserData2;
            HighlightLayer = event.UserEvent.UserData3;
            return true;
        case ECE_PLAY_INTRO_MUSIC:
            if (AudioDriver)
                AudioDriver->playMusic("mus_intro.ogg", 1.0f, false);
            return true;
        case ECE_START_SHUTTLE_RACE:
            NextState = EGS_SHUTTLE_RACE;
            return true;
        case ECE_ATTACK_STARTED:
        case ECE_QUICK_SAVE:
        case ECE_QUICK_LOAD:
        case ECE_START_SHUTTLE_RACE + 1:
            return true;
        default:
            return false;
        }
    default:
        return false;
    }
}

void CPlayState::setBuildAction(int index)
{
    entity::SBuildingInfoItem* info = entity::gp_buildableItems->getBuildingInfo(index);
    if (info && info->Enabled)
    {
        Action = 1;
        RecycleTarget = 0;
        clearSelectedEntity();
        if (RecycleButton)
            RecycleButton->setChecked(false);
        PlacementOk = false;
        CursorMoved = true;
        BuildSelection = index;
    }
}

void CPlayState::setGameSpeed(int speed)
{
    GameSpeed = speed;
    addInfoLine(settings::gp_systemConfig->getLocalizedText(GAME_SPEED_NAMES[speed]));
}

void CPlayState::setNoneAction(bool clearSelection)
{
    Action = 0;
    RecycleTarget = 0;
    if (RecycleButton)
        RecycleButton->setChecked(false);
    if (clearSelection)
        clearSelectedEntity();
}

void CPlayState::replaceSelectedMissileTurret(int entityType)
{
    int index = BuildableItems.getIndexForEntityType(entityType);
    int cost = BuildableItems.getEntityMineralCost(index);
    if (game::gp_mineralAmount->getValue() < cost)
        return;

    game::gp_mineralAmount->modifyValue(-cost);
    game::gp_negatedMineralAmount->modifyValue(cost);
    ox::core::CVector3d<float> position = SelectedEntity->getPosition();
    int id = SelectedEntity->getId();
    SelectedEntity->killEntity();
    entity::CConstructionEntity* construction =
        new entity::CConstructionEntity(position.X, position.Y, BuildableItems.getEntityId(index));
    construction->setId(id);
    entity::gp_entityManager->appendEntity(construction, 0);
}

void CPlayState::makeSelectedDeathstarTower()
{
    if (SelectedEntity && SelectedEntity->getEntityType() == 7)
    {
        ox::TArray<ox::entity::COxEntity*> towers;
        entity::gp_entityManager->getAllEntitiesInRange(towers,
            ox::core::CPosition2d<float>(SelectedEntity->getPosition().X, SelectedEntity->getPosition().Y), 40000.0f, 0,
            7);
        for (unsigned int i = 0; i < towers.size(); ++i)
        {
            entity::CDefenseTowerEntity* tower = (entity::CDefenseTowerEntity*)towers[i];
            if (!tower->isLinked())
                tower->handleSelectionDraggedToEntity(SelectedEntity);
        }
    }
}

void CPlayState::unmakeSelectedDeathstarTower()
{
    if (SelectedEntity && SelectedEntity->getEntityType() == 7)
    {
        ox::TArray<ox::entity::COxEntity*> towers;
        entity::gp_entityManager->getAllEntitiesInRange(towers,
            ox::core::CPosition2d<float>(SelectedEntity->getPosition().X, SelectedEntity->getPosition().Y), 40000.0f, 0,
            7);
        for (unsigned int i = 0; i < towers.size(); ++i)
        {
            entity::CDefenseTowerEntity* tower = (entity::CDefenseTowerEntity*)towers[i];
            if (tower != SelectedEntity && tower->isLinkedTo((entity::CDefenseTowerEntity*)SelectedEntity))
                tower->handleSelectionDraggedToNothing();
        }
    }
}

void CPlayState::sellAllHarvesters()
{
    const std::list<ox::entity::COxEntity*>& buildings = entity::gp_entityManager->getEntityList(0);
    for (std::list<ox::entity::COxEntity*>::const_iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        // Mineral gatherers that have run out of minerals.
        if ((*it)->getEntityType() == 4 && !((entity::CMineralGatherEntity*)*it)->hasMoreMinerals())
            sellEntity((entity::CEntity*)*it);
    }
    setNoneAction(true);
}

void CPlayState::replaceSelectedProducer(int entityType)
{
    int index = BuildableItems.getIndexForEntityType(entityType);
    int cost = BuildableItems.getEntityMineralCost(index) / 2;
    if (game::gp_mineralAmount->getValue() < cost)
        return;

    game::gp_mineralAmount->modifyValue(-cost);
    game::gp_negatedMineralAmount->modifyValue(cost);
    ox::core::CVector3d<float> position = SelectedEntity->getPosition();
    int id = SelectedEntity->getId();
    SelectedEntity->killEntity();
    entity::CConstructionEntity* construction =
        new entity::CConstructionEntity(position.X, position.Y, BuildableItems.getEntityId(index));
    construction->setId(id);
    entity::gp_entityManager->appendEntity(construction, 0);
}

void CPlayState::toggleWaveList()
{
    setWaveListToggle(!ListVisible[LIST_WAVES]);
}

void CPlayState::toggleCreativeList()
{
    setCreativeListToggle(!ListVisible[LIST_CREATIVE]);
}

void CPlayState::launchWaveLevel(int wave)
{
    if (ThreatLevel)
    {
        ox::core::CString<wchar_t> aliens;
        ThreatLevel->spawnNextWaveAttack(wave, aliens);

        game::SInfoLineMessage message;
        message.Name = settings::gp_systemConfig->getLocalizedText(L"ingame:waveWarningTitle");
        message.Portrait = "PortraitCommunications";
        message.Sound = "ingame_infoWave.ogg";
        message.Text = settings::gp_systemConfig->getLocalizedText(L"ingame:waveWarningMessage");
        addInfoLine(&message, false);
    }
    // Rebuilds the wave list without the launched wave.
    toggleWaveList();
    toggleWaveList();
    BuildingAttacked = false;
}

void CPlayState::setPlaceAlienAction(int alienType)
{
    Action = 3;
    RecycleTarget = 0;
    clearSelectedEntity();
    PlacementOk = false;
    CursorMoved = true;
    AlienSelection = alienType;
}

void CPlayState::replaceBuildingWithType(entity::CEntity* building, const char* entityId)
{
    if (!building || building->getEntityType() == 5)
        return;

    entity::SBuildingInfoItem* info = entity::gp_buildableItems->getBuildingInfoByEntityId(entityId);
    if (!info)
        return;

    int cost = info->MineralCost.getValue();
    if (game::gp_mineralAmount->getValue() < cost)
        return;

    game::gp_mineralAmount->modifyValue(-cost);
    game::gp_negatedMineralAmount->modifyValue(cost);
    ox::core::CVector3d<float> position = building->getPosition();
    int id = building->getId();
    building->killEntity();
    entity::CConstructionEntity* construction = new entity::CConstructionEntity(position.X, position.Y, entityId);
    construction->setId(id);
    entity::gp_entityManager->appendEntity(construction, 0);
}

void CPlayState::togglePause()
{
    if (GameSpeed == 0 && m_2c0 == 0)
        m_2c0 = 3;
    else if (GameSpeed != 0 && m_2c0 != 0)
        m_2c0 = 0;
    int speed = GameSpeed;
    GameSpeed = m_2c0;
    m_2c0 = speed;
    setGameSpeed(GameSpeed);
}

void CPlayState::renderMinimap()
{
    ox::core::CRect<int> top = TopBar->getAbsolutePosition();
    if (ShowMinimap && GuiSprites[GS_MINIMAP_BACKGROUND])
    {
        // The frame around the minimap, the edges tiled between the corners.
        ox::core::CDimension2d<int> cornerSize = GuiSprites[GS_MINIMAP_TOP_LEFT]->getFrameSize(0);
        ox::core::CPosition2d<int> topLeft(top.LowerRightCorner.X - MinimapWidth, top.UpperLeftCorner.Y);
        GuiSprites[GS_MINIMAP_TOP_LEFT]->draw(topLeft, 0, ox::video::SColor(0xffffffff));
        ox::core::CPosition2d<int> topRight(top.LowerRightCorner.X - cornerSize.Width, top.UpperLeftCorner.Y);
        GuiSprites[GS_MINIMAP_TOP_RIGHT]->draw(topRight, 0, ox::video::SColor(0xffffffff));
        ox::core::CRect<int> topEdge(cornerSize.Width + topLeft.X, top.UpperLeftCorner.Y, topRight.X,
            cornerSize.Height + top.UpperLeftCorner.Y);
        ox::core::CDimension2d<int> tileSize = GuiSprites[GS_MINIMAP_TOP]->getFrameSize(0);
        for (int x = topEdge.UpperLeftCorner.X; x < topEdge.LowerRightCorner.X; x += tileSize.Width)
            GuiSprites[GS_MINIMAP_TOP]->draw(ox::core::CPosition2d<int>(x, top.UpperLeftCorner.Y), &topEdge,
                ox::video::SColor(0xffffffff));

        cornerSize = GuiSprites[GS_MINIMAP_BOTTOM_LEFT]->getFrameSize(0);
        ox::core::CPosition2d<int> bottomLeft(top.LowerRightCorner.X - MinimapWidth,
            MinimapHeight + top.UpperLeftCorner.Y - cornerSize.Height);
        GuiSprites[GS_MINIMAP_BOTTOM_LEFT]->draw(bottomLeft, 0, ox::video::SColor(0xffffffff));
        ox::core::CPosition2d<int> bottomRight(top.LowerRightCorner.X - cornerSize.Width, bottomLeft.Y);
        GuiSprites[GS_MINIMAP_BOTTOM_RIGHT]->draw(bottomRight, 0, ox::video::SColor(0xffffffff));
        ox::core::CRect<int> bottomEdge(cornerSize.Width + bottomLeft.X, bottomLeft.Y, bottomRight.X,
            cornerSize.Height + bottomLeft.Y);
        tileSize = GuiSprites[GS_MINIMAP_BOTTOM]->getFrameSize(0);
        for (int x = bottomEdge.UpperLeftCorner.X; x < bottomEdge.LowerRightCorner.X; x += tileSize.Width)
            GuiSprites[GS_MINIMAP_BOTTOM]->draw(ox::core::CPosition2d<int>(x, bottomLeft.Y), &bottomEdge,
                ox::video::SColor(0xffffffff));

        ox::core::CDimension2d<int> sideSize = GuiSprites[GS_MINIMAP_LEFT]->getFrameSize(0);
        ox::core::CRect<int> leftEdge(topLeft.X, topEdge.LowerRightCorner.Y, sideSize.Width + topLeft.X, bottomLeft.Y);
        GuiSprites[GS_MINIMAP_LEFT]->draw(leftEdge.UpperLeftCorner, &leftEdge, ox::video::SColor(0xffffffff));
        ox::core::CRect<int> rightEdge(top.LowerRightCorner.X - sideSize.Width, topEdge.LowerRightCorner.Y,
            top.LowerRightCorner.X, bottomLeft.Y);
        GuiSprites[GS_MINIMAP_RIGHT]->draw(rightEdge.UpperLeftCorner, &rightEdge, ox::video::SColor(0xffffffff));

        tileSize = GuiSprites[GS_MINIMAP_BACKGROUND]->getFrameSize(0);
        ox::core::CRect<int> background(leftEdge.LowerRightCorner.X, leftEdge.UpperLeftCorner.Y,
            rightEdge.UpperLeftCorner.X, leftEdge.LowerRightCorner.Y);
        for (int x = background.UpperLeftCorner.X; x < background.LowerRightCorner.X; x += tileSize.Width)
            GuiSprites[GS_MINIMAP_BACKGROUND]->draw(ox::core::CPosition2d<int>(x, background.UpperLeftCorner.Y),
                &background, ox::video::SColor(0xffffffff));

        ox::core::CDimension2d<int> mineralsSize = GuiSprites[GS_MINERALS_BACKGROUND]->getFrameSize(0);
        GuiSprites[GS_MINERALS_BACKGROUND]->draw(
            ox::core::CPosition2d<int>(topLeft.X - mineralsSize.Width, top.UpperLeftCorner.Y), 0,
            ox::video::SColor(0xffffffff));
    }
    else if (GuiSprites[GS_TOP_RIGHT_BACKGROUND])
    {
        // The second planet has a bright ground, so the small minimap gets a dark backing.
        if (game::gp_world->getPlanet() == 1)
            Driver->draw2DRectangle(ox::video::SColor(0x80000000), MinimapRect, 0);
        ox::core::CDimension2d<int> size = GuiSprites[GS_TOP_RIGHT_BACKGROUND]->getFrameSize(0);
        GuiSprites[GS_TOP_RIGHT_BACKGROUND]->draw(
            ox::core::CPosition2d<int>(top.LowerRightCorner.X - size.Width, top.UpperLeftCorner.Y), 0,
            ox::video::SColor(0xffffffff));
    }

    if (displayThreatLevelForGameMode(GameMode) && GuiSprites[GS_THREAT_LEVEL_BACKGROUND])
    {
        GuiSprites[GS_THREAT_LEVEL_BACKGROUND]->draw(
            ox::core::CPosition2d<int>(top.UpperLeftCorner.X + TimerWidth, top.UpperLeftCorner.Y), 0,
            ox::video::SColor(0xffffffff));
        float progress = ThreatLevel->getThreatLevelProgress();
        if (GameMode == game::EGM_CREATIVE && LuaManager && LuaManager->isRunningMods())
            progress = LuaManager->getThreatLevelProgress();
        if (progress > 0)
        {
            ox::core::CDimension2d<int> size = GuiSprites[GS_PROGRESS_BAR]->getFrameSize(0);
            ox::core::CPosition2d<int> position(top.UpperLeftCorner.X + TimerWidth + 13, top.UpperLeftCorner.Y + 4);
            size.Width = (int)(size.Width * progress + 0.5f);
            ox::core::CRect<int> clip(position, size);
            GuiSprites[GS_PROGRESS_BAR]->draw(position, &clip, THREAT_TEXT_COLOR);
        }
    }

    if (GameMode == game::EGM_CAMPAIGN && GuiSprites[GS_OBJECTIVES_BACKGROUND])
    {
        ox::core::CPosition2d<int> position(top.UpperLeftCorner.X + 8, top.UpperLeftCorner.Y + 19);
        GuiSprites[GS_OBJECTIVES_BACKGROUND]->draw(position, 0, ox::video::SColor(0xffffffff));
        if (Scenario && GuiSprites[GS_ICON_OBJECTIVE])
        {
            for (int i = 0; i < Scenario->getNumObjectives(); ++i)
            {
                ox::core::CPosition2d<int> icon(position.X + 20, position.Y + 30 + i * 20);
                GuiSprites[GS_ICON_OBJECTIVE]->draw(icon, 0, ox::video::SColor(0xffffffff));
                SmallFont->draw(Scenario->getObjective(i).c_str(),
                    ox::core::CRect<int>(icon.X + 15, icon.Y - 6, icon.X + 215, icon.Y + 14),
                    ox::video::SColor(0xc0ffffff), ox::gui::EFHA_LEFT, ox::gui::EFVA_TOP, 0);
            }
        }
    }

    if (displayTimerForGameMode(GameMode) && GuiSprites[GS_TIME_BACKGROUND])
    {
        GuiSprites[GS_TIME_BACKGROUND]->draw(top.UpperLeftCorner, 0, ox::video::SColor(0xffffffff));
        ox::core::CString<wchar_t> text;
        if (GameMode == game::EGM_CREATIVE && LuaManager)
            text = ox::core::CStringFunctions::millisecondsToWide(LuaManager->getTimerValue(), true);
        else
            text = ox::core::CStringFunctions::millisecondsToWide(GameTime, true);
        ox::core::CDimension2d<int> size = GuiSprites[GS_TIME_BACKGROUND]->getFrameSize(0);
        BoldFont->draw(text.c_str(),
            ox::core::CRect<int>(top.UpperLeftCorner.X + 30, top.UpperLeftCorner.Y, size.Width + top.UpperLeftCorner.X,
                top.UpperLeftCorner.Y + size.Height),
            ox::video::SColor(0xffffffff), ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER, 0);
    }

    if ((GameMode == game::EGM_RUSH || (GameMode == game::EGM_CREATIVE && LuaManager && LuaManager->isRushListVisible())) &&
        GuiSprites[GS_DAMAGE_BAR_BACKGROUND])
    {
        ox::core::CDimension2d<int> size = GuiSprites[GS_DAMAGE_BAR_BACKGROUND]->getFrameSize(0);
        ox::core::CPosition2d<int> position(ScreenSize.Width - size.Width, TopBar->getAbsolutePosition().LowerRightCorner.Y + 10);
        GuiSprites[GS_DAMAGE_BAR_BACKGROUND]->draw(position, 0, ox::video::SColor(0xffffffff));
        float damage = game::gp_statistics->getRushModeDamage();
        if (GameMode == game::EGM_CREATIVE && LuaManager)
            damage = LuaManager->getRushProgress() * 50000.0f;
        if (damage > 0 && GuiSprites[GS_DAMAGE_BAR])
        {
            size = GuiSprites[GS_DAMAGE_BAR]->getFrameSize(0);
            position.X += 40;
            position.Y += 42;
            ox::core::CRect<int> clip(position.X, (int)(size.Height * (damage / -50000.0f + 1.0f)) + position.Y,
                size.Width + position.X, position.Y + size.Height);
            GuiSprites[GS_DAMAGE_BAR]->draw(position, &clip, ox::video::SColor(0xffffffff));
        }
    }

    // Flash the credits while there are not enough of them.
    if (DenialTime > 0 && (int)(DenialTime * 1000.0f) / 250 % 2 && CreditsText)
    {
        ox::core::CRect<int> area = CreditsText->getAbsolutePosition();
        area.UpperLeftCorner.X -= 24;
        area.LowerRightCorner.X += 28;
        Driver->draw2DRectangle(ox::video::SColor(0x806060cc), area, 0);
    }

    if (!MinimapDot || (!MinimapTexture && UseMinimapTexture))
        return;

    const ox::core::CRect<float>& field = game::gp_world->getVisibleGameFieldSize();
    ox::core::CRect<int> area = MinimapRect;
    float scaleX = (float)MinimapRect.getWidth() / field.getWidth();
    float scaleY = (float)MinimapRect.getHeight() / field.getHeight();
    if (MinimapUpdateTime <= 0 || !UseMinimapTexture)
    {
        MinimapUpdateTime = 2.0f;
        if (UseMinimapTexture)
            area = ox::core::CRect<int>(0, 0, MinimapRect.getWidth(), MinimapRect.getHeight());

        int selectedType = -1;
        if (SelectedEntity)
            selectedType = SelectedEntity->getEntityType();
        if (UseMinimapTexture)
            Driver->setRenderTarget(MinimapTexture, true, true, ox::video::SColor(0));

        const std::list<ox::entity::COxEntity*>& buildings = entity::gp_entityManager->getEntityList(0);
        for (std::list<ox::entity::COxEntity*>::const_iterator it = buildings.begin(); it != buildings.end(); ++it)
        {
            const ox::core::CVector3d<float>& position = (*it)->getPosition();
            if (!field.isPointInside(ox::core::CPosition2d<float>(position.X, position.Y)))
                continue;

            // The selected building's kind stands out; minerals are cyan.
            ox::video::SColor color(0xffffffff);
            if ((*it)->getEntityType() != selectedType)
                color = (*it)->getEntityType() == 5 ? ox::video::SColor(0xff20c0c0) :
                    ox::video::SColor(selectedType < 0 ? 0xff20ff20 : 0x4020ff20);
            MinimapDot->draw(ox::core::CPosition2d<int>((int)((position.X - field.UpperLeftCorner.X) * scaleX) +
                                                            area.UpperLeftCorner.X,
                                 (int)((position.Y - field.UpperLeftCorner.Y) * scaleY) + area.UpperLeftCorner.Y),
                &area, color);
        }

        const std::list<ox::entity::COxEntity*>& aliens = entity::gp_entityManager->getEntityList(1);
        for (std::list<ox::entity::COxEntity*>::const_iterator it = aliens.begin(); it != aliens.end(); ++it)
        {
            const ox::core::CVector3d<float>& position = (*it)->getPosition();
            if (!field.isPointInside(ox::core::CPosition2d<float>(position.X, position.Y)))
                continue;

            MinimapDot->draw(ox::core::CPosition2d<int>((int)((position.X - field.UpperLeftCorner.X) * scaleX) +
                                                            area.UpperLeftCorner.X,
                                 (int)((position.Y - field.UpperLeftCorner.Y) * scaleY) + area.UpperLeftCorner.Y),
                &area, ox::video::SColor(0xffff2020));
        }

        if (UseMinimapTexture)
            Driver->setRenderTarget(0, false, false, ox::video::SColor(0));
    }

    if (UseMinimapTexture)
        Driver->draw2DImage(MinimapTexture, MinimapRect.UpperLeftCorner,
            ox::core::CRect<int>(0, 0, MinimapRect.getWidth(), MinimapRect.getHeight()), &MinimapRect,
            (ox::video::SColor*)0, true);

    for (std::list<SMinimapMarker>::iterator it = MinimapMarkers.begin(); it != MinimapMarkers.end(); ++it)
    {
        int x = (int)((it->X - field.UpperLeftCorner.X) * scaleX) + area.UpperLeftCorner.X;
        int y = (int)((it->Y - field.UpperLeftCorner.Y) * scaleY) + area.UpperLeftCorner.Y;
        ox::core::CRect<int> box;
        if (it->Time > 4.0f)
        {
            int size = (int)((it->Time - 4.0f) * 100.0f) + 6;
            box.UpperLeftCorner = ox::core::CPosition2d<int>(x - size / 2, y - size / 2);
            box.LowerRightCorner = box.UpperLeftCorner + ox::core::CPosition2d<int>(size, size);
        }
        else
            box = ox::core::CRect<int>(x - 3, y - 3, x + 4, y + 4);

        // Clip the square to the minimap and skip the edges that were cut.
        bool left = true;
        if (box.UpperLeftCorner.X < area.UpperLeftCorner.X)
        {
            box.UpperLeftCorner.X = area.UpperLeftCorner.X;
            left = false;
        }
        bool top = true;
        if (box.UpperLeftCorner.Y < area.UpperLeftCorner.Y)
        {
            box.UpperLeftCorner.Y = area.UpperLeftCorner.Y;
            top = false;
        }
        bool right = true;
        if (box.LowerRightCorner.X >= area.LowerRightCorner.X)
        {
            box.LowerRightCorner.X = area.LowerRightCorner.X - 1;
            right = false;
        }
        bool bottom = true;
        if (box.LowerRightCorner.Y >= area.LowerRightCorner.Y)
        {
            box.LowerRightCorner.Y = area.LowerRightCorner.Y - 1;
            bottom = false;
        }
        if (top)
            Driver->draw2DLine(box.UpperLeftCorner,
                ox::core::CPosition2d<int>(box.LowerRightCorner.X, box.UpperLeftCorner.Y), MINIMAP_MARKER_COLOR);
        if (left)
            Driver->draw2DLine(box.UpperLeftCorner,
                ox::core::CPosition2d<int>(box.UpperLeftCorner.X, box.LowerRightCorner.Y), MINIMAP_MARKER_COLOR);
        if (right)
            Driver->draw2DLine(box.LowerRightCorner,
                ox::core::CPosition2d<int>(box.LowerRightCorner.X, box.UpperLeftCorner.Y), MINIMAP_MARKER_COLOR);
        if (bottom)
            Driver->draw2DLine(box.LowerRightCorner,
                ox::core::CPosition2d<int>(box.UpperLeftCorner.X, box.LowerRightCorner.Y), MINIMAP_MARKER_COLOR);
    }

    // The part of the world on screen.
    ox::core::CPosition2d<int> view((int)((ViewPosition.X - field.UpperLeftCorner.X) * scaleX + 0.5f) +
                                        MinimapRect.UpperLeftCorner.X,
        (int)((ViewPosition.Y - field.UpperLeftCorner.Y) * scaleY + 0.5f) + MinimapRect.UpperLeftCorner.Y);
    ox::core::CDimension2d<int> viewSize = getViewSize();
    viewSize.Width = (int)(viewSize.Width * scaleX) - 1;
    viewSize.Height = (int)(viewSize.Height * scaleY) - 1;
    Driver->draw2DLine(view, ox::core::CPosition2d<int>(view.X + viewSize.Width, view.Y), ox::video::SColor(0xc0ffffff));
    Driver->draw2DLine(view, ox::core::CPosition2d<int>(view.X, view.Y + viewSize.Height), ox::video::SColor(0xc0ffffff));
    Driver->draw2DLine(ox::core::CPosition2d<int>(view.X + viewSize.Width, view.Y + viewSize.Height),
        ox::core::CPosition2d<int>(view.X + viewSize.Width, view.Y), ox::video::SColor(0xc0ffffff));
    Driver->draw2DLine(ox::core::CPosition2d<int>(view.X, view.Y + viewSize.Height),
        ox::core::CPosition2d<int>(view.X + viewSize.Width, view.Y + viewSize.Height), ox::video::SColor(0xc0ffffff));
}

void CPlayState::renderWaveButton(ox::gui::IGUIElement* button, int numAliens, int firstAlien, bool* aliens,
    const ox::core::CString<wchar_t>& text)
{
    if (numAliens <= 0)
        return;

    ox::core::CRect<int> rect = button->getAbsolutePosition();
    ox::core::CPosition2d<int> center((rect.UpperLeftCorner.X + 20 + rect.LowerRightCorner.X) / 2,
        (rect.UpperLeftCorner.Y + rect.LowerRightCorner.Y) / 2);
    if (numAliens == 1)
    {
        if (WaveIcons[firstAlien])
            WaveIcons[firstAlien]->draw(center, 0, ox::video::SColor(0xffffffff));
    }
    else
    {
        int x = rect.UpperLeftCorner.X + 40;
        for (int i = firstAlien; i < WAVE_COUNT; ++i)
        {
            if (aliens[i] && WaveIcons[i])
            {
                WaveIcons[i]->draw(ox::core::CPosition2d<int>(x, center.Y), 0, ox::video::SColor(0xffffffff));
                x += 20 / (numAliens - 1);
            }
        }
    }

    if (text.size() > 0 && NumberFont)
    {
        rect.LowerRightCorner.X = rect.UpperLeftCorner.X + 20;
        NumberFont->draw(text.c_str(), rect, ox::video::SColor(0xffc86464), ox::gui::EFHA_CENTER,
            ox::gui::EFVA_CENTER, 0);
    }
}

void CPlayState::setRecycleAction()
{
    Action = 2;
    RecycleTarget = 0;
    clearSelectedEntity();
    PlacementOk = false;
    CursorMoved = true;
}

ox::core::CPosition2d<float> CPlayState::getWorldPos(ox::core::CPosition2d<int> position)
{
    return ox::core::CPosition2d<float>(position.X + ViewPosition.X, position.Y + ViewPosition.Y);
}

ox::core::CDimension2d<int> CPlayState::getViewSize()
{
    int width = ScreenSize.Width;
    int height = ScreenSize.Height;
    if (GuiSprites[GS_BOTTOM_LEFT_BACKGROUND])
        height -= GuiSprites[GS_BOTTOM_LEFT_BACKGROUND]->getFrameSize(0).Height;
    return ox::core::CDimension2d<int>(width, height);
}

void CPlayState::performRectangleSelection(ox::core::CPosition2d<float> corner1,
    ox::core::CPosition2d<float> corner2, bool add, int entityType)
{
    if (!add)
        clearSelectedEntity();

    ox::core::CRect<float> rect(corner1, corner2);
    repairRect(rect);

    const std::list<ox::entity::COxEntity*>& buildings = entity::gp_entityManager->getEntityList(0);
    bool selectedNothing = true;
    bool skippedSome = false;
    for (std::list<ox::entity::COxEntity*>::const_iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        int type = (*it)->getEntityType();
        // Minerals are only selected when the rectangle holds nothing else.
        if (type != 5 && (type == entityType || entityType < 0))
        {
            const ox::core::CVector3d<float>& position = (*it)->getPosition();
            if (position.X >= rect.UpperLeftCorner.X && position.X <= rect.LowerRightCorner.X &&
                position.Y >= rect.UpperLeftCorner.Y && position.Y <= rect.LowerRightCorner.Y)
            {
                bool selected = false;
                if (add)
                {
                    for (ox::TArray<ox::entity::SEntityReference*>::iterator s = MultiSelection.begin();
                         s != MultiSelection.end(); ++s)
                    {
                        if ((*s)->Entity == *it)
                        {
                            selected = true;
                            break;
                        }
                    }
                }
                if (!selected)
                    addToSelection((entity::CEntity*)*it);
                selectedNothing = false;
            }
        }
        else
            skippedSome = true;
    }

    if (skippedSome && selectedNothing)
    {
        for (std::list<ox::entity::COxEntity*>::const_iterator it = buildings.begin(); it != buildings.end(); ++it)
        {
            if ((*it)->getEntityType() != 5)
                continue;

            const ox::core::CVector3d<float>& position = (*it)->getPosition();
            if (position.X >= rect.UpperLeftCorner.X && position.X <= rect.LowerRightCorner.X &&
                position.Y >= rect.UpperLeftCorner.Y && position.Y <= rect.LowerRightCorner.Y)
            {
                bool selected = false;
                if (add)
                {
                    for (ox::TArray<ox::entity::SEntityReference*>::iterator s = MultiSelection.begin();
                         s != MultiSelection.end(); ++s)
                    {
                        if ((*s)->Entity == *it)
                        {
                            selected = true;
                            break;
                        }
                    }
                }
                if (!selected)
                    addToSelection((entity::CEntity*)*it);
            }
        }
    }

    if (MultiSelection.size() == 1)
    {
        SelectedEntity = (entity::CEntity*)MultiSelection[0]->Entity;
        newSelectedEntity();
    }
}

void CPlayState::buildingShiftSelected(entity::CEntity* building)
{
    if (SelectedEntity)
    {
        entity::CEntity* selected = SelectedEntity;
        if (selected == building)
        {
            // Shift-clicking the selection deselects it.
            clearSelectedEntity();
            return;
        }
        clearSelectedEntity();
        addToSelection(selected);
        addToSelection(building);
        return;
    }

    if (MultiSelection.empty())
    {
        SelectedEntity = building;
        newSelectedEntity();
        return;
    }

    for (ox::TArray<ox::entity::SEntityReference*>::iterator it = MultiSelection.begin(); it != MultiSelection.end();
         ++it)
    {
        if ((*it)->Entity == building)
        {
            delete *it;
            MultiSelection.erase(it);
            if (MultiSelection.size() == 1)
            {
                SelectedEntity = (entity::CEntity*)MultiSelection[0]->Entity;
                newSelectedEntity();
            }
            return;
        }
    }
    addToSelection(building);
}

void CPlayState::sellEntity(entity::CEntity* building)
{
    if (!building || building->isKilled())
        return;

    int value = building->getSellValue();
    ox::core::CVector3d<float> position = building->getPosition();
    int credits = building->getSellValue();
    game::gp_mineralAmount->modifyValue(credits);
    game::gp_negatedMineralAmount->modifyValue(-credits);
    building->killEntity();
    for (int i = 0; i < value; ++i)
        entity::gp_entityManager->appendEntity(
            new entity::CParticleEntity(position.X, position.Y, position.Z, 0, "SellEvent"), 4);

    if (LuaManager)
        LuaManager->hookBuildingSold(entity::gp_buildableItems->getEntityIdForEntityInstance(building),
            building->getPosition().X, building->getPosition().Y, value);
}

void CPlayState::placeCurrentAlienSelection(const ox::core::CPosition2d<float>& position)
{
    entity::CAlienEntity* alien = entity::gp_entityManager->addAlien(AlienSelection, position.X, position.Y);
    if (LuaManager)
        LuaManager->hookAlienSpawned(alien);
}

void CPlayState::setWaveListToggle(bool visible)
{
    int y = TopBar->getAbsolutePosition().LowerRightCorner.Y + 10;
    if (!ListGroups[LIST_WAVES])
    {
        ListGroups[LIST_WAVES] = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 50, 50), 0);
        ListButtons[LIST_WAVES] =
            GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 50, 50), ListGroups[LIST_WAVES], 36, 0);
        ListButtons[LIST_WAVES]->setAnimations(IngamePackage, "WaveSendBtn", true);
        ListButtons[LIST_WAVES]->LayoutFlags = "br";
        ListContents[LIST_WAVES] =
            GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 50, 50), ListGroups[LIST_WAVES]);
        ListContents[LIST_WAVES]->LayoutFlags = "br";
        ListGroups[LIST_WAVES]->sortRiver(true, 0, 1, false);
        ListGroups[LIST_WAVES]->moveTo(ox::core::CPosition2d<int>(
            ScreenSize.Width - 1 - ListGroups[LIST_WAVES]->getRelativePosition().getWidth(), y));
        loadWaveListSprites();
    }

    if (ListVisible[LIST_WAVES] == visible)
        return;

    ListVisible[LIST_WAVES] = visible;
    if (visible)
    {
        if (ListContents[LIST_WAVES])
        {
            for (int i = 0; i < 10; ++i)
            {
                if (ThreatLevel->hasWaveBeenLaunched(i))
                    continue;
                if (LuaManager && !LuaManager->isWaveButtonVisible(i))
                    continue;

                ox::gui::IGUIButton* button =
                    GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 50, 50), ListContents[LIST_WAVES], i + 38, 0);
                button->setAnimations(IngamePackage, "WaveBtn", true);
                button->LayoutFlags = "br";
                button->setReportOnDraw(2);
                if (GameMode == game::EGM_WAVE)
                {
                    ox::gui::IGUIWindow* popup = static_cast<ox::gui::IGUIWindow*>(GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 50, 50),
                        GUIEnvironment->getHoverParentElement(), -1));
                    popup->setAnimations(IngamePackage, "Tooltip");
                    GUIEnvironment->addStaticText(ThreatLevel->getWaveDescription(i),
                        ox::core::CRect<int>(0, 0, 400, 400), false, true, popup, -1, L"")->packSize();
                    popup->sortRiver(true, 5, 5, false);
                    button->setHoverItem(popup);
                }
            }
            ListContents[LIST_WAVES]->sortRiver(true, 0, 1, false);
        }
        ListGroups[LIST_WAVES]->sortRiver(true, 0, 1, false);
        ox::core::CRect<int> area = ListGroups[LIST_WAVES]->getRelativePosition();
        int offset = 0;
        if (ListGroups[LIST_CREATIVE] && LuaManager && LuaManager->isCreativeListVisible())
            offset = -ListGroups[LIST_CREATIVE]->getRelativePosition().getWidth() - 1;
        if (GuiSprites[GS_DAMAGE_BAR_BACKGROUND] && LuaManager && LuaManager->isRushListVisible())
            offset += -GuiSprites[GS_DAMAGE_BAR_BACKGROUND]->getFrameSize(0).Width - 1;
        ListGroups[LIST_WAVES]->moveTo(ox::core::CPosition2d<int>(ScreenSize.Width - 1 - area.getWidth() + offset, y));
    }
    else
    {
        if (ListContents[LIST_WAVES])
        {
            ListContents[LIST_WAVES]->removeAllChildren();
            ListGroups[LIST_WAVES]->sortRiver(true, 0, 1, false);
        }
        if (ListGroups[LIST_WAVES])
        {
            ListGroups[LIST_WAVES]->sortRiver(true, 0, 0, false);
            ox::core::CRect<int> area = ListGroups[LIST_WAVES]->getRelativePosition();
            int offset = 0;
            if (ListGroups[LIST_CREATIVE] && LuaManager && LuaManager->isCreativeListVisible())
                offset = -ListGroups[LIST_CREATIVE]->getRelativePosition().getWidth() - 1;
            if (GuiSprites[GS_DAMAGE_BAR_BACKGROUND] && LuaManager && LuaManager->isRushListVisible())
                offset += -GuiSprites[GS_DAMAGE_BAR_BACKGROUND]->getFrameSize(0).Width - 1;
            ListGroups[LIST_WAVES]->moveTo(
                ox::core::CPosition2d<int>(ScreenSize.Width - 1 - area.getWidth() + offset, y));
        }
    }
}

void CPlayState::setCreativeListToggle(bool visible)
{
    int y = TopBar->getAbsolutePosition().LowerRightCorner.Y + 10;
    if (!ListGroups[LIST_CREATIVE])
    {
        ListGroups[LIST_CREATIVE] = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 50, 50), 0);
        ListButtons[LIST_CREATIVE] =
            GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 50, 50), ListGroups[LIST_CREATIVE], 37, 0);
        ListButtons[LIST_CREATIVE]->setAnimations(IngamePackage, "CreativePlaceBtn", true);
        ListButtons[LIST_CREATIVE]->LayoutFlags = "br";
        ListContents[LIST_CREATIVE] =
            GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 50, 50), ListGroups[LIST_CREATIVE]);
        ListContents[LIST_CREATIVE]->LayoutFlags = "br";
        ListGroups[LIST_CREATIVE]->sortRiver(true, 0, 1, false);
        ListGroups[LIST_CREATIVE]->moveTo(ox::core::CPosition2d<int>(
            ScreenSize.Width - 1 - ListGroups[LIST_CREATIVE]->getRelativePosition().getWidth(), y));
        loadWaveListSprites();
    }

    if (ListVisible[LIST_CREATIVE] == visible)
        return;

    ListVisible[LIST_CREATIVE] = visible;
    if (visible)
    {
        if (ListContents[LIST_CREATIVE])
        {
            for (int i = 0; i < WAVE_COUNT; ++i)
            {
                if (!ThreatLevel->alienIsPresentAtThisLevel(i))
                    continue;

                ox::gui::IGUIButton* button = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 50, 50),
                    ListContents[LIST_CREATIVE], i + 48, 0);
                button->setAnimations(IngamePackage, "WaveBtn", true);
                button->LayoutFlags = "br";
                button->setReportOnDraw(2);
                ox::gui::IGUIWindow* popup = static_cast<ox::gui::IGUIWindow*>(GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 50, 50),
                    GUIEnvironment->getHoverParentElement(), -1));
                popup->setAnimations(IngamePackage, "Tooltip");
                ox::core::CString<wchar_t> text =
                    settings::gp_systemConfig->getLocalizedText(entity::ALIEN_KEY_NAMES[i]);
                text = settings::gp_systemConfig->getLocalizedText(L"ingame:drawAlien", text.c_str());
                GUIEnvironment->addStaticText(text.c_str(), ox::core::CRect<int>(0, 0, 400, 400), false, true, popup,
                    -1, L"")->packSize();
                popup->sortRiver(true, 5, 5, false);
                button->setHoverItem(popup);
            }
            ListContents[LIST_CREATIVE]->sortRiver(true, 0, 1, false);
        }
        ListGroups[LIST_CREATIVE]->sortRiver(true, 0, 1, false);
        ox::core::CRect<int> area = ListGroups[LIST_CREATIVE]->getRelativePosition();
        int offset = 0;
        if (GuiSprites[GS_DAMAGE_BAR_BACKGROUND] && LuaManager && LuaManager->isRushListVisible())
            offset = -GuiSprites[GS_DAMAGE_BAR_BACKGROUND]->getFrameSize(0).Width - 1;
        ListGroups[LIST_CREATIVE]->moveTo(
            ox::core::CPosition2d<int>(ScreenSize.Width - 1 - area.getWidth() + offset, y));
    }
    else
    {
        if (ListContents[LIST_CREATIVE])
        {
            ListContents[LIST_CREATIVE]->removeAllChildren();
            ListGroups[LIST_CREATIVE]->sortRiver(true, 0, 1, false);
        }
        if (ListGroups[LIST_CREATIVE])
        {
            ListGroups[LIST_CREATIVE]->sortRiver(true, 0, 0, false);
            ox::core::CRect<int> area = ListGroups[LIST_CREATIVE]->getRelativePosition();
            int offset = 0;
            if (GuiSprites[GS_DAMAGE_BAR_BACKGROUND] && LuaManager && LuaManager->isRushListVisible())
                offset = -GuiSprites[GS_DAMAGE_BAR_BACKGROUND]->getFrameSize(0).Width - 1;
            ListGroups[LIST_CREATIVE]->moveTo(
                ox::core::CPosition2d<int>(ScreenSize.Width - 1 - area.getWidth() + offset, y));
        }
    }
}

bool CPlayState::isMultiSelected()
{
    return !MultiSelection.empty();
}

void CPlayState::changeConstructionSelection(bool next)
{
    BuildSelection = BuildableItems.changeConstructionSelection(BuildSelection, next);
    setBuildAction(BuildSelection);
}

void CPlayState::addInfoLine(game::SInfoLineMessage* message, bool sound)
{
    if (InfoLines)
    {
        ox::video::ISpritePackage* package = sound ? MenuPackage : IngamePackage;
        InfoLines->addInfoLine(message->Name.c_str(), message->Text.c_str(), package, message->Portrait.c_str());
        if (AudioDriver)
        {
            if (message->Sound.size() > 0)
                AudioDriver->playVoice(message->Sound.c_str());
            else
                AudioDriver->playSound("message.ogg", 1.0f, 0.0f, 1.0f);
        }
    }
}

bool CPlayState::writeStateToFile(const char* filename, const wchar_t* description)
{
    ox::io::CMemWriteFile* file = new ox::io::CMemWriteFile();
    ox::io::CHelpIO::writeInt(file, StartTime);
    ox::io::CHelpIO::writeInt(file, RandomValue);
    ox::io::CHelpIO::writeWideString(file, PlayerName, true);
    ox::io::CHelpIO::writeWideString(file, PlayerGroup, true);
    ox::io::CHelpIO::writeFloat(file, ViewPosition.X);
    ox::io::CHelpIO::writeFloat(file, ViewPosition.Y);
    ox::io::CHelpIO::writeInt(file, GameMode);
    ThreatLevel->write(file);
    ox::io::CHelpIO::writeInt(file, entity::g_nextEntityId);
    game::gp_mineralAmount->write(file);
    ox::io::CHelpIO::writeFloat(file, GameTime);
    ox::io::CHelpIO::writeInt(file, Victory);
    ox::io::CHelpIO::writeInt(file, BuildingAttacked);
    if (LuaManager)
    {
        ox::io::CHelpIO::writeByte(file, 1);
        LuaManager->writeLuaStates(file);
    }
    else
        ox::io::CHelpIO::writeByte(file, 0);
    game::gp_world->write(file);
    game::gp_statistics->write(file);
    for (int i = 0; i < 5; ++i)
        settings::g_attackPriorities[i].write(file);
    entity::gp_entityManager->writeEntities(file);

    int size = file->getSize();
    unsigned char* compressed = new unsigned char[size];
    unsigned int compressedSize = 0;
    Device->getFileSystem()->zipDeflateData(compressed, size, (unsigned char*)file->getData(), size, compressedSize);
    delete file;

    ox::core::CAes aes;
    ox::algo::CRand random(1);
    char keyData[32];
    for (int i = 0; i < 32; ++i)
        keyData[i] = random.nextInt(256);
    ox::core::CCipherKey key(keyData, 32);
    aes.setKey(&key);

    int encryptedSize = aes.getEncryptedDataSize(compressedSize);
    unsigned char* encrypted = new unsigned char[encryptedSize];
    aes.encrypt(encrypted, compressed, compressedSize);
    delete[] compressed;

    bool result;
    ox::io::IWriteFile* saveFile = Device->getFileSystem()->createAndWriteFile(filename, false);
    if (saveFile)
    {
        settings::SSavestateHeader header;
        header.PlayerName = PlayerName;
        header.GameMode = GameMode;
        header.Unknown = Minerals;
        header.ThreatLevel = ThreatLevel->getThreatLevel();
        header.Planet = game::gp_world->getPlanet();
        header.Time = time(0);
        header.Description = description;
        settings::CSavestateInfo::writeHeader(saveFile, header);
        ox::io::CHelpIO::writeInt(saveFile, compressedSize);
        ox::io::CHelpIO::writeInt(saveFile, size);
        ox::io::CHelpIO::writeInt(saveFile, encryptedSize);
        saveFile->write(encrypted, encryptedSize);
        delete[] encrypted;
        saveFile->drop();
        result = true;
    }
    else
    {
        result = false;
        delete[] encrypted;
    }
    return result;
}

void CPlayState::addInfoLine(const ox::core::CString<wchar_t>& text)
{
    if (InfoLines)
        InfoLines->addInfoLine(text.c_str());
}

void CPlayState::toggleMenuPos()
{
}

void CPlayState::renderFirst()
{
    if (LoadingScreen)
        LoadingScreen->render(Device, Driver);
}

void CPlayState::render()
{
    unsigned int startTime = Device->getTimer()->getTime();
    Driver->beginScene(true, true, ox::video::SColor(0xff000000));
    if (game::gp_world)
        game::gp_world->renderBackground(ViewPosition, BoldFont, 0);

    ox::core::CRect<int> viewPort(ox::core::CPosition2d<int>(0, 0), ScreenSize);
    if (ShowAllRanges && RangeCircle)
    {
        const std::list<ox::entity::COxEntity*>& entities = entity::gp_entityManager->getEntityList(0);
        std::list<ox::entity::COxEntity*>::const_iterator it = entities.begin();
        if (SelectedEntity)
        {
            // Only the buildings of the selected type.
            int type = SelectedEntity->getEntityType();
            for (; it != entities.end(); ++it)
                if (type == (*it)->getEntityType())
                    renderRangeCircleForEntity(*it, viewPort);
        }
        else
        {
            for (; it != entities.end(); ++it)
                renderRangeCircleForEntity(*it, viewPort);
        }
    }

    if (Action == 1)
    {
        int entityType = BuildableItems.getEntityType(BuildSelection);
        if (Beam180.Beam)
        {
            if (Beam210.Beam)
            {
                Beam180.Start = PlacementPosition;
                Beam210.Start = PlacementPosition;
                ox::TArray<ox::entity::COxEntity*> buildings;
                entity::gp_entityManager->getAllRangeLineBuildings(buildings, PlacementPosition, entityType);
                for (unsigned int i = 0; i < buildings.size(); ++i)
                {
                    if (buildings[i]->getEntityType() == 5)
                    {
                        const ox::core::CVector3d<float>& end = buildings[i]->getPosition();
                        Beam210.End = ox::core::CPosition2d<float>(end.X, end.Y);
                        entity::gp_entityManager->renderEnergyBeam(&Beam210, ViewPosition, viewPort);
                    }
                    else
                    {
                        const ox::core::CVector3d<float>& end = buildings[i]->getPosition();
                        Beam180.End = ox::core::CPosition2d<float>(end.X, end.Y);
                        entity::gp_entityManager->renderEnergyBeam(&Beam180, ViewPosition, viewPort);
                    }
                }
            }
            if (Beam180.Beam && game::gp_world->worldChangesSizeInThisGameMode())
            {
                // Outlines the part of the field the world grows into.
                const ox::core::CRect<float>& field = game::gp_world->getActualGameFieldSize();
                float left = field.UpperLeftCorner.X + 512.0f;
                float top = field.UpperLeftCorner.Y + 512.0f;
                float right = field.LowerRightCorner.X - 512.0f;
                float bottom = field.LowerRightCorner.Y - 512.0f;
                Beam180.Start = ox::core::CPosition2d<float>(left, top);
                Beam180.End = ox::core::CPosition2d<float>(right, top);
                entity::gp_entityManager->renderEnergyBeam(&Beam180, ViewPosition, viewPort);
                Beam180.Start = Beam180.End;
                Beam180.End = ox::core::CPosition2d<float>(right, bottom);
                entity::gp_entityManager->renderEnergyBeam(&Beam180, ViewPosition, viewPort);
                Beam180.Start = Beam180.End;
                Beam180.End = ox::core::CPosition2d<float>(left, bottom);
                entity::gp_entityManager->renderEnergyBeam(&Beam180, ViewPosition, viewPort);
                Beam180.Start = Beam180.End;
                Beam180.End = ox::core::CPosition2d<float>(left, top);
                entity::gp_entityManager->renderEnergyBeam(&Beam180, ViewPosition, viewPort);
            }
        }
        if (PlacementOk && RangeCircle && m_2a0)
        {
            if (PlacementOk)
                renderRangeCircle(PlacementPosition, 150.0f, RANGE_CIRCLE_COLOR, viewPort);
            if (entityType == 7)
                renderRangeCircle(PlacementPosition, 200.0f, LASER_CIRCLE_COLOR, viewPort);
            else if (entityType == 4)
                renderRangeCircle(PlacementPosition, 100.0f, MINING_CIRCLE_COLOR, viewPort);
        }
        if (PlacementOk && BuildableItems.getPreviewSprite(BuildSelection))
        {
            ox::core::CPosition2d<float> position(viewPort.UpperLeftCorner.X + PlacementPosition.X - ViewPosition.X,
                viewPort.UpperLeftCorner.Y + PlacementPosition.Y - ViewPosition.Y);
            BuildableItems.getPreviewSprite(BuildSelection)->drawScaled(position, 1.0f,
                ox::video::SColor(0x80ffffff));
        }
    }
    else if (Action == 3)
    {
        if ((unsigned int)AlienSelection < WAVE_COUNT && m_488[AlienSelection])
        {
            ox::core::CPosition2d<float> mouse = getWorldPos(GUIEnvironment->getMousePosition());
            ox::core::CPosition2d<float> position(viewPort.UpperLeftCorner.X + mouse.X - ViewPosition.X,
                viewPort.UpperLeftCorner.Y + mouse.Y - ViewPosition.Y);
            m_488[AlienSelection]->drawScaled(position, 1.0f, ox::video::SColor(0x80ffffff));
        }
    }
    else
    {
        if (SelectedEntity)
        {
            if (RangeCircle)
                renderRangeCircleForEntity(SelectedEntity, viewPort);
            if (HasLastPlacement && SelectedEntity->getEntityType() == 1)
            {
                // The energy redirection being dragged from a spark mover.
                const ox::core::CVector3d<float>& start = SelectedEntity->getPosition();
                Beam1c8.Start = ox::core::CPosition2d<float>(start.X, start.Y);
                Beam1c8.End = getWorldPos(GUIEnvironment->getMousePosition());
                entity::gp_entityManager->renderEnergyBeam(&Beam1c8, ViewPosition, viewPort);
            }
        }
    }

    if (Selector)
    {
        if (SelectedEntity)
        {
            ox::core::CPosition2d<float> position(SelectedEntity->getPosition().X - ViewPosition.X,
                SelectedEntity->getPosition().Y - ViewPosition.Y);
            float scale = SelectedEntity->getCollisionSize() * 1.5f / 25.0f;
            Selector->drawScaled(position, scale, ox::video::SColor(0xffffffff));
        }
        else
        {
            for (ox::TArray<ox::entity::SEntityReference*>::iterator it = MultiSelection.begin();
                 it != MultiSelection.end(); ++it)
            {
                entity::CEntity* selected = (entity::CEntity*)(*it)->Entity;
                ox::core::CPosition2d<float> position(selected->getPosition().X - ViewPosition.X,
                    selected->getPosition().Y - ViewPosition.Y);
                float scale = selected->getCollisionSize() * 1.5f / 25.0f;
                Selector->drawScaled(position, scale, ox::video::SColor(0xffffffff));
            }
        }
    }

    if (entity::gp_entityManager)
        entity::gp_entityManager->renderEntities(ViewPosition, viewPort);

    if (RectangleSelecting)
    {
        // The selection rectangle.
        float offsetY = viewPort.UpperLeftCorner.Y - ViewPosition.Y;
        float cornerY = LastPlacement.Y + offsetY;
        float offsetX = viewPort.UpperLeftCorner.X - ViewPosition.X;
        float cornerX = LastPlacement.X + offsetX;
        ox::core::CPosition2d<float> mouse = getWorldPos(GUIEnvironment->getMousePosition());
        Driver->draw2DLine(ox::core::CPosition2d<int>((int)cornerX, (int)cornerY),
            ox::core::CPosition2d<int>((int)(mouse.X + offsetX), (int)cornerY), RANGE_LINE_COLOR);
        Driver->draw2DLine(ox::core::CPosition2d<int>((int)cornerX, (int)cornerY),
            ox::core::CPosition2d<int>((int)cornerX, (int)(mouse.Y + offsetY)), RANGE_LINE_COLOR);
        Driver->draw2DLine(ox::core::CPosition2d<int>((int)cornerX, (int)(mouse.Y + offsetY)),
            ox::core::CPosition2d<int>((int)(mouse.X + offsetX), (int)(mouse.Y + offsetY)), RANGE_LINE_COLOR);
        Driver->draw2DLine(ox::core::CPosition2d<int>((int)(mouse.X + offsetX), (int)cornerY),
            ox::core::CPosition2d<int>((int)(mouse.X + offsetX), (int)(mouse.Y + offsetY)), RANGE_LINE_COLOR);
    }

    if (game::gp_world)
        game::gp_world->renderEdgeShades(ViewPosition);

    // Blacks out the screen beyond the edges of the field.
    ox::core::CRect<float> field = game::gp_world->getVisibleGameFieldSize();
    if (ScreenSizeF.Width + ViewPosition.X > field.LowerRightCorner.X)
        Driver->draw2DRectangle(ox::video::SColor(0xff000000),
            ox::core::CRect<int>((int)(field.LowerRightCorner.X - ViewPosition.X), 0, ScreenSize.Width,
                ScreenSize.Height), 0);
    if (ScreenSizeF.Height + ViewPosition.Y > field.LowerRightCorner.Y)
        Driver->draw2DRectangle(ox::video::SColor(0xff000000),
            ox::core::CRect<int>(0, (int)(field.LowerRightCorner.Y - ViewPosition.Y), ScreenSize.Width,
                ScreenSize.Height), 0);
    if (field.UpperLeftCorner.X > ViewPosition.X)
        Driver->draw2DRectangle(ox::video::SColor(0xff000000),
            ox::core::CRect<int>(0, 0, (int)(field.UpperLeftCorner.X - ViewPosition.X), ScreenSize.Height), 0);
    if (field.UpperLeftCorner.Y > ViewPosition.Y)
        Driver->draw2DRectangle(ox::video::SColor(0xff000000),
            ox::core::CRect<int>(0, 0, ScreenSize.Width, (int)(field.UpperLeftCorner.Y - ViewPosition.Y)), 0);

    if (RecycleTarget && Action == 2 && RecycleSelector)
    {
        // The recycle selector and the minerals recycling gives.
        float x = RecycleTarget->getPosition().X - ViewPosition.X;
        float y = RecycleTarget->getPosition().Y - ViewPosition.Y;
        RecycleSelector->drawScaled(ox::core::CPosition2d<float>(x - 21.5f, y - 17.0f), 1.0f,
            ox::video::SColor(0xffffffff));
        ox::core::CString<wchar_t> text(L"+");
        text.append(RecycleTarget->getSellValue());
        BoldFont->draw(text.c_str(),
            ox::core::CRect<int>((int)x + 15, (int)y - 22, (int)x + 45, BoldFontHeight + (int)y - 22),
            ox::video::SColor(0xffa7c0ff), ox::gui::EFHA_LEFT, ox::gui::EFVA_TOP, 0);
    }

    if (HighlightActive)
        gui::CGuiEffects::renderRecangleOverlay(Driver,
            static_cast<const ox::TList<ox::entity::COxEntity*>&>(
                entity::gp_entityManager->getEntityList(HighlightLayer)),
            ViewPosition, HighlightEntityType, 40);

    if (CreditsText && ThreatLevelText)
    {
        ox::core::CString<wchar_t> text;
        text = ox::core::CString<wchar_t>(game::gp_mineralAmount->getValue());
        CreditsText->setText(text.c_str());
        if (GameMode == game::EGM_CREATIVE)
        {
            if (LuaManager && LuaManager->isRunningMods())
                text = ox::core::CString<wchar_t>(LuaManager->getThreatLevelValue());
            else
                // Creative games count the aliens.
                text = ox::core::CString<wchar_t>((int)entity::gp_entityManager->getEntityList(1).size());
        }
        else
            text = ox::core::CString<wchar_t>(ThreatLevel->getThreatLevel());
        ThreatLevelText->setText(text.c_str());
    }

    if (GameOver)
    {
        // The screen fades to black.
        int alpha = (int)((GameOverTime / -15.0f + 1.0f) * 255.0f);
        alpha = ox::core::clamp(alpha, 0, 255);
        Driver->draw2DRectangle(ox::video::SColor(alpha, 0, 0, 0),
            ox::core::CRect<int>(ox::core::CPosition2d<int>(0, 0), ScreenSize), 0);
        ox::core::CString<wchar_t> text(L"--- ");
        if (GameWon)
            text.append(settings::gp_systemConfig->getLocalizedText(L"ingame:victorySplash"));
        else
            text.append(settings::gp_systemConfig->getLocalizedText(L"ingame:gameoverSplash"));
        text.append(ox::core::CString<wchar_t>(L" ---"));
        BoldFont->draw(text.c_str(), ox::core::CRect<int>(ox::core::CPosition2d<int>(0, 0), ScreenSize),
            WHITE_TEXT_COLOR, ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER, 0);
    }
    else if (GameSpeed == 0)
    {
        ox::core::CString<wchar_t> text(L"--- ");
        text.append(settings::gp_systemConfig->getLocalizedText(L"ingame:pausedSplash"));
        text.append(ox::core::CString<wchar_t>(L" ---"));
        BoldFont->draw(text.c_str(), ox::core::CRect<int>(ox::core::CPosition2d<int>(0, 0), ScreenSize),
            WHITE_TEXT_COLOR, ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER, 0);
    }

    if (LuaManager)
        LuaManager->renderGuiObjects(Driver, SmallFont);
    GUIEnvironment->drawAll();

    if (ShowDebugInfo)
    {
        ox::core::CString<wchar_t> text(L"FPS:\t");
        text.append(Driver->getFPS());
        BoldFont->draw(text.c_str(),
            ox::core::CRect<int>(10, ScreenSize.Height / 2 - 20, 210, ScreenSize.Height / 2),
            ox::video::SColor(0xffffffff), ox::gui::EFHA_LEFT, ox::gui::EFVA_TOP, 0);
        text = L"Logic time (ms):\t";
        text.append(UpdateDuration);
        BoldFont->draw(text.c_str(),
            ox::core::CRect<int>(10, ScreenSize.Height / 2, 210, ScreenSize.Height / 2 + 20),
            ox::video::SColor(0xffffffff), ox::gui::EFHA_LEFT, ox::gui::EFVA_TOP, 0);
        text = L"Render time (ms):\t";
        text.append(RenderDuration);
        BoldFont->draw(text.c_str(),
            ox::core::CRect<int>(10, ScreenSize.Height / 2 + 20, 210, ScreenSize.Height / 2 + 40),
            ox::video::SColor(0xffffffff), ox::gui::EFHA_LEFT, ox::gui::EFVA_TOP, 0);
    }
    if (ShowMouseWorldPos)
    {
        ox::core::CString<wchar_t> text(L"Mouse world pos:\t");
        ox::core::CPosition2d<float> mouse = getWorldPos(GUIEnvironment->getMousePosition());
        text.append((int)mouse.X);
        text.append(ox::core::CString<wchar_t>(L", "));
        text.append((int)mouse.Y);
        BoldFont->draw(text.c_str(),
            ox::core::CRect<int>(10, ScreenSize.Height / 2 + 40, 210, ScreenSize.Height / 2 + 60),
            ox::video::SColor(0xffffffff), ox::gui::EFHA_LEFT, ox::gui::EFVA_TOP, 0);
    }

    Driver->endScene();
    RenderDuration = Device->getTimer()->getTime() - startTime;
}

void CPlayState::renderRangeCircleForEntity(ox::entity::COxEntity* entity, const ox::core::CRect<int>& viewPort)
{
    switch (entity->getEntityType())
    {
    case 0:
    case 1:
        renderRangeCircle(ox::core::CPosition2d<float>(entity->getPosition().X, entity->getPosition().Y), 150.0f,
            RANGE_CIRCLE_COLOR, viewPort);
        break;
    case 4:
        renderRangeCircle(ox::core::CPosition2d<float>(entity->getPosition().X, entity->getPosition().Y), 100.0f,
            MINING_CIRCLE_COLOR, viewPort);
        break;
    case 7:
        renderRangeCircle(ox::core::CPosition2d<float>(entity->getPosition().X, entity->getPosition().Y),
            ((entity::CDefenseTowerEntity*)entity)->getTotalRange(), LASER_CIRCLE_COLOR, viewPort);
        break;
    case 8:
        renderRangeCircle(ox::core::CPosition2d<float>(entity->getPosition().X, entity->getPosition().Y), 500.0f,
            LASER_CIRCLE_COLOR, viewPort);
        break;
    case 14:
        renderRangeCircle(ox::core::CPosition2d<float>(entity->getPosition().X, entity->getPosition().Y), 500.0f,
            LASER_CIRCLE_COLOR, viewPort);
        break;
    case 13:
        renderRangeCircle(ox::core::CPosition2d<float>(entity->getPosition().X, entity->getPosition().Y), 1500.0f,
            LASER_CIRCLE_COLOR, viewPort);
        break;
    }
}

void CPlayState::renderRangeCircle(const ox::core::CPosition2d<float>& position, float range, ox::video::SColor color,
    const ox::core::CRect<int>& viewPort)
{
    // Small circles are a scaled sprite, large ones a ring of beams.
    if (range < 250.0f && RangeCircle)
    {
        RangeCircle->drawScaled(ox::core::CPosition2d<float>(position.X - ViewPosition.X + viewPort.UpperLeftCorner.X,
                                    position.Y - ViewPosition.Y + viewPort.UpperLeftCorner.Y),
            (range + range) / 255.0f, color);
        return;
    }

    Beam258.Color = color;
    Beam258.Start.X = position.X;
    Beam258.Start.Y = position.Y - range;
    for (int i = 1; i != 66; ++i)
    {
        float angle = i * -2.0f * 0.0483321957f + 1.57079637f;
        Beam258.End.X = position.X + cos((double)angle) * range;
        Beam258.End.Y = position.Y - sin((double)angle) * range;
        entity::gp_entityManager->renderEnergyBeam(&Beam258, ViewPosition, viewPort);
        Beam258.Start = Beam258.End;
    }
}

void CPlayState::addParticleEntity(ox::video::IParticleState* state, const ox::core::CVector3d<float>& position)
{
    if (!state)
        return;

    if (state->getParticleImportance() > 0 &&
        !VisibleArea.isPointInside(ox::core::CPosition2d<float>(position.X, position.Y + position.Z)))
    {
        state->remove();
        return;
    }

    if (entity::gp_entityManager->getEntityList(4).size() < 10000)
        entity::gp_entityManager->appendEntity(new entity::CParticleEntity(state, position), 4);
    else
        state->remove();
}

void CPlayState::playParticleSound(const char* sound, const ox::core::CVector3d<float>& position)
{
    if (AudioDriver)
    {
        float dx = position.X - (ScreenSizeF.Width * 0.5f + ViewPosition.X);
        float pan = ox::core::clamp(dx, -ScreenSizeF.Width, ScreenSizeF.Width);
        float dy = position.Y - (ScreenSizeF.Height * 0.5f + ViewPosition.Y);
        float distance = dx * dx + dy * dy;
        float range = ScreenSizeF.Height * ScreenSizeF.Width;
        if (distance <= range)
            AudioDriver->playSound(sound, 1.0f - distance / range, pan / ScreenSizeF.Width, 1.0f);
    }
}

void CPlayState::eraseGameObjects()
{
    delete game::gp_world;
    game::createCredits();
    delete entity::gp_entityManager;
    delete ThreatLevel;
    delete game::gp_statistics;
    game::gp_world = 0;
    entity::gp_entityManager = 0;
    ThreatLevel = 0;
    game::gp_statistics = 0;
    PlacementOk = false;
    GameOver = false;
    HasLastPlacement = false;
    DraggingFromSelection = false;
    setNoneAction(true);
    g_scenarioResult = 0;
}

void CPlayState::displayWelcomeMessage()
{
    game::SInfoLineMessage message;
    message.Name = settings::gp_systemConfig->getLocalizedText(L"characters:pilot");
    message.Portrait = "PortraitDropShip";
    switch (GameMode)
    {
    case game::EGM_NORMAL:
    case game::EGM_INSANE:
    {
        int levels = settings::gp_profileManager->getCurrentProfile()->getLocalScore(0, GameMode,
            game::gp_world->getPlanet());
        int minerals = settings::gp_profileManager->getCurrentProfile()->getLocalScore(1, GameMode,
            game::gp_world->getPlanet());
        if (levels == 0 || minerals == 0)
        {
            message.Text = settings::gp_systemConfig->getLocalizedText(L"ingame:welcomeNoScores");
            message.Sound = "welcomeGoodLuck.ogg";
        }
        else
        {
            message.Text = settings::gp_systemConfig->getLocalizedText(L"ingame:welcomeLevelScores", levels, minerals);
            message.Sound = "welcomeLevelScores.ogg";
        }
        break;
    }
    case game::EGM_WAVE:
    case game::EGM_RUSH:
    {
        int time = settings::gp_profileManager->getCurrentProfile()->getLocalScore(2, GameMode,
            game::gp_world->getPlanet());
        if (time)
        {
            ox::core::CString<wchar_t> record = ox::core::CStringFunctions::millisecondsToWide(time * 0.001f, true);
            message.Text = settings::gp_systemConfig->getLocalizedText(L"ingame:welcomeTimeScores", record.c_str());
            message.Sound = "welcomeLevelScores.ogg";
        }
        else
        {
            message.Text = settings::gp_systemConfig->getLocalizedText(L"ingame:welcomeNoScores");
            message.Sound = "welcomeGoodLuck.ogg";
        }
        break;
    }
    case game::EGM_CREATIVE:
        message.Text = settings::gp_systemConfig->getLocalizedText(L"ingame:welcomeCreative");
        message.Sound = "welcomeCreative.ogg";
        break;
    }
    addInfoLine(&message, false);
}

void CPlayState::loadWaveListSprites()
{
    WaveIcons[0] = IngamePackage->addNewAnimationState("WaveIconMilky");
    WaveIcons[1] = IngamePackage->addNewAnimationState("WaveIconShielder");
    WaveIcons[2] = IngamePackage->addNewAnimationState("WaveIconTiny");
    WaveIcons[3] = IngamePackage->addNewAnimationState("WaveIconSummoner");
    WaveIcons[4] = IngamePackage->addNewAnimationState("WaveIconLooker");
    WaveIcons[5] = IngamePackage->addNewAnimationState("WaveIconHogger");
    WaveIcons[6] = IngamePackage->addNewAnimationState("WaveIconStealer");
    WaveIcons[7] = IngamePackage->addNewAnimationState("WaveIconBrain");
    WaveIcons[8] = IngamePackage->addNewAnimationState("WaveIconMega");
}

void CPlayState::addToSelection(entity::CEntity* building)
{
    entity::CEntity* selected = SelectedEntity;
    if (selected && MultiSelection.empty())
    {
        clearSelectedEntity();
        addToSelection(selected);
    }

    ox::entity::SEntityReference* reference = new ox::entity::SEntityReference();
    reference->Entity = building;
    reference->Id = building->getId();
    reference->UpdateCounter = entity::gp_entityManager->getUpdateCounter();
    MultiSelection.push_back(reference);
}

int CPlayState::getOnDieMarkerAtPos(const ox::core::CVector3d<float>& position)
{
    return 0;
}

} // end namespace states
} // end namespace harvest
