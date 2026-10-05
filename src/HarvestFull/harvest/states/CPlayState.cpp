// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <math.h>
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
#include "harvest/entity/CDefenseTowerEntity.h"
#include "harvest/entity/CEntityManager.h"
#include "harvest/entity/CMinerEntity.h"
#include "harvest/game/CLuaManager.h"
#include "harvest/game/CScenario.h"
#include "harvest/game/CStatistics.h"
#include "harvest/game/CThreatLevel.h"
#include "harvest/game/SInfoLineMessage.h"
#include "harvest/gui/CAchievementsScreen.h"
#include "harvest/gui/CGuiInfoLines.h"
#include "harvest/gui/CIngameMenuScreen.h"
#include "harvest/gui/CPriorityScreen.h"
#include "harvest/gui/CSaveGameScreen.h"
#include "harvest/gui/CSettingsScreen.h"
#include "harvest/gui/CStoryScreen.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/IOxDevice.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/core/CBasic.h"
#include "ox/core/CHiddenInt.h"
#include "ox/core/CMath.h"
#include "ox/gui/IGUICheckBox.h"
#include "ox/gui/IGUIElement.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/gui/IGUILayout.h"
#include "ox/gui/IGUIStaticText.h"
#include "ox/video/IParticleState.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {
namespace game {

//! Replaces the credit counters with new empty ones.
static void createCredits()
{
    delete gp_mineralAmount;
    gp_mineralAmount = new ox::core::CHiddenInt();
    delete gp_negatedMineralAmount;
    gp_negatedMineralAmount = new ox::core::CHiddenInt();
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
    L"gamespeed:normal", L"gamespeed:threehalfs", L"gamespeed:double"};

bool CPlayState::m_keys[256];

bool displayThreatLevelForGameMode(int gameMode)
{
    return gameMode == game::EGM_NORMAL || gameMode == game::EGM_INSANE || gameMode == game::EGM_SHUTTLE_RACE ||
        gameMode == game::EGM_CREATIVE;
}

bool displayTimerForGameMode(int gameMode)
{
    if (gameMode == game::EGM_CREATIVE && game::gp_luaManager)
        return game::gp_luaManager->isTimerVisible();
    return gameMode == game::EGM_WAVE || gameMode == game::EGM_RUSH;
}

CPlayState::CPlayState()
    : m_054(false), LoadingScreen(0), InitStep(0), BoldFont(0), SmallFont(0), NumberFont(0), IngamePackage(0),
      MenuPackage(0), m_0c0(1), m_0c4(1), Action(0), SelectedEntity(0),
      RecycleTarget(0), m_0f0(0), m_0f8(-1), m_0fc(0), m_104(false), HasLastPlacement(false), m_111(false),
      m_130(false), m_131(false), PlacementOk(false), BuildSelection(0), RangeCircle(0), Selector(0),
      RecycleSelector(0), Beam180(6.0f), Beam1c8(2.0f), Beam210(6.0f), Beam258(6.0f), m_2a0(false), ThreatLevel(0),
      Scenario(0), GameSpeed(3), m_2c0(0), m_2c4(false), m_2c5(false), m_2c8(0), m_2d0(false), MinimapDot(0),
      m_2f0(0), m_2f8(false), m_2fc(0), m_310(0), m_314(0), m_318(0), GameTime(0), m_320(false), m_321(false),
      m_322(false), m_324(0), DenialTime(0), m_340(0), m_348(0), m_350(0), m_4f8(0), m_500(0), m_508(0), m_514(0),
      m_518(0), RecycleButton(0), m_728(0), m_758(false), m_764(false), m_768(0), m_76c(0), m_770(0), m_774(false),
      m_7b0(0), SettingsScreen(0), PriorityScreen(0), IngameMenuScreen(0), SaveGameScreen(0), StoryScreen(0),
      AchievementsScreen(0), InfoLines(0), m_7f8(0), m_820(2), m_824(1.0f), m_828(false), m_829(false), m_82c(0),
      m_830(0), m_838(0)
{
    for (int i = 0; i < 256; ++i)
        m_keys[i] = false;
    for (int i = 0; i < GS_COUNT; ++i)
        GuiSprites[i] = 0;
    for (int i = 0; i < WAVE_COUNT; ++i)
    {
        m_418[i] = 0;
        m_488[i] = 0;
    }
    for (int i = 0; i < 2; ++i)
    {
        m_7a8[i] = false;
        m_778[i] = 0;
        m_788[i] = 0;
        m_798[i] = 0;
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

    if (m_2f0)
    {
        m_2f0->drop();
        m_2f0 = 0;
    }
    if (MinimapDot)
        MinimapDot->remove();
    if (RangeCircle)
        RangeCircle->remove();
    if (Selector)
        Selector->remove();
    if (RecycleSelector)
        RecycleSelector->remove();
    if (m_4f8)
        m_4f8->remove();
    if (m_500)
        m_500->remove();
    if (m_508)
        m_508->remove();
    if (m_7b0)
        m_7b0->remove();
    for (int i = 0; i < 2; ++i)
        if (m_778[i])
            m_778[i]->remove();
    for (int i = 0; i < GS_COUNT; ++i)
        if (GuiSprites[i])
            GuiSprites[i]->remove();
    for (int i = 0; i < WAVE_COUNT; ++i)
    {
        if (m_418[i])
            m_418[i]->remove();
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

ox::gui::IGUILayout* CPlayState::getPopupForGuiButton(const wchar_t* text)
{
    ox::gui::IGUILayout* popup =
        GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 90, 10), GUIEnvironment->getRootGUIElement(), -1);
    popup->setAnimations(IngamePackage, "Tooltip");
    ox::gui::IGUIStaticText* label = GUIEnvironment->addStaticText(text, ox::core::CRect<int>(0, 0, 1000, 1000),
        false, true, popup, -1, 0);
    label->setOverrideColor(WHITE_TEXT_COLOR);
    label->setOverrideFont(BoldFont);
    label->packSize();
    popup->sortRiver(true, 5, 5, false);
    return popup;
}

void CPlayState::playPlanetMusic()
{
    if (game::gp_world && GameMode != game::EGM_SHUTTLE_RACE)
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
    m_330 = 1200.0f;
}

void CPlayState::clearSelectedEntity()
{
    SelectedEntity = 0;
    MultiSelection.clear();
    if (m_728)
        m_728->setText(L"");
    if (m_730)
        m_730->setText(L"");
    if (m_738)
        m_738->setText(L"");
    if (m_740)
        m_740->setText(L"");
    if (m_500)
        m_500->setVisible(false);
    if (LuaManager)
        LuaManager->updateSelectedBuilding(0);
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

void CPlayState::updatePlacementPosition()
{
    m_133 = false;
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
    m_133 = false;
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
        m_133 = true;
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
    setWaveListToggle(!m_7a8[0]);
}

void CPlayState::toggleCreativeList()
{
    setCreativeListToggle(!m_7a8[1]);
}

void CPlayState::setPlaceAlienAction(int alienType)
{
    Action = 3;
    RecycleTarget = 0;
    clearSelectedEntity();
    PlacementOk = false;
    m_133 = true;
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
        if (m_418[firstAlien])
            m_418[firstAlien]->draw(center, 0, ox::video::SColor(0xffffffff));
    }
    else
    {
        int x = rect.UpperLeftCorner.X + 40;
        for (int i = firstAlien; i < WAVE_COUNT; ++i)
        {
            if (aliens[i] && m_418[i])
            {
                m_418[i]->draw(ox::core::CPosition2d<int>(x, center.Y), 0, ox::video::SColor(0xffffffff));
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
    m_133 = true;
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
        height -= GuiSprites[GS_BOTTOM_LEFT_BACKGROUND]->getFrameSize(0).Y;
    return ox::core::CDimension2d<int>(width, height);
}

void CPlayState::performRectangleSelection(ox::core::CPosition2d<float> corner1,
    ox::core::CPosition2d<float> corner2, bool add, int entityType)
{
    if (!add)
        clearSelectedEntity();

    ox::core::CRect<float> rect(corner1, corner2);
    rect.repair();

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
        clearSelectedEntity();
        if (selected == building)
            return;
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
        InfoLines->addInfoLine(message->Name.c_str(), message->Text.c_str(), sound ? MenuPackage : IngamePackage,
            message->Portrait.c_str());
        if (AudioDriver)
        {
            if (message->Sound.size() > 0)
                AudioDriver->playVoice(message->Sound.c_str());
            else
                AudioDriver->playSound("message.ogg", 1.0f, 0.0f, 1.0f);
        }
    }
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
    m_2c4 = false;
    HasLastPlacement = false;
    m_111 = false;
    setNoneAction(true);
    g_scenarioResult = 0;
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
