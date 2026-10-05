// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <math.h>
#include <time.h>
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
#include "ox/gui/IGUICheckBox.h"
#include "ox/gui/IGUIElement.h"
#include "ox/gui/IGUIButton.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/gui/IGUILayout.h"
#include "ox/gui/IGUIStaticText.h"
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
    : m_054(false), LoadingScreen(0), InitStep(0), BoldFont(0), SmallFont(0), NumberFont(0), IngamePackage(0),
      MenuPackage(0), MousePosition(1, 1), Action(0), SelectedEntity(0),
      RecycleTarget(0), FollowJump(false), HasLastPlacement(false), m_111(false),
      m_130(false), m_131(false), PlacementOk(false), BuildSelection(0), RangeCircle(0), Selector(0),
      RecycleSelector(0), Beam180(6.0f), Beam1c8(2.0f), Beam210(6.0f), Beam258(6.0f), m_2a0(false), ThreatLevel(0),
      Scenario(0), GameSpeed(3), m_2c0(0), GameOver(false), GameWon(false), GameOverTime(0), m_2d0(false), MinimapDot(0),
      MinimapTexture(0), UseMinimapTexture(false), MinimapUpdateTime(0), StatsTime(0), HarvestingCount(0), OverheatedCount(0), GameTime(0), Victory(false), LevelRecordShown(false),
      MineralsRecordShown(false), RecordCheckTime(0), DenialTime(0), CreditsText(0), HarvestersText(0), ThreatLevelText(0), BottomBar(0), ActionPanel(0), TopBar(0), MinimapWidth(0),
      MinimapHeight(0), RecycleButton(0), InfoText(0), BuildingsScrolling(false), m_764(false), m_768(0), m_76c(0), m_770(0), m_774(false),
      m_7b0(0), SettingsScreen(0), PriorityScreen(0), IngameMenuScreen(0), SaveGameScreen(0), StoryScreen(0),
      AchievementsScreen(0), InfoLines(0), Profile(0), ParticleSetting(2), ScrollSpeed(1.0f), m_828(false), m_829(false), UpdateDuration(0),
      m_830(0), m_838(0)
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

ox::gui::IGUILayout* CPlayState::getPopupForBuildButton(const wchar_t* name, const wchar_t* description, int energy,
    int minerals)
{
    ox::gui::IGUILayout* popup =
        GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 100, 100), GUIEnvironment->getRootGUIElement());
    ox::gui::IGUILayout* titleFrame = GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 20, 20), popup, -1);
    titleFrame->setAnimations(IngamePackage, "Tooltip");
    ox::gui::IGUILayout* mineralsFrame =
        GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 20, 20), popup, GUI_ID_MINERALS_POPUP);
    mineralsFrame->setAnimations(IngamePackage, "Tooltip");
    mineralsFrame->setReportOnDraw(1);
    ox::gui::IGUILayout* energyFrame =
        GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 20, 20), popup, GUI_ID_ENERGY_POPUP);
    energyFrame->setAnimations(IngamePackage, "Tooltip");
    energyFrame->setReportOnDraw(1);
    ox::gui::IGUILayout* descriptionFrame = GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 240, 23), popup, -1);
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
        unsigned int scramble = 0x4f2c7b19;
        for (int i = 0; i < compressedSize; ++i)
        {
            switch (i & 3)
            {
            case 0:
                data[i] ^= scramble >> 24;
                break;
            case 1:
                data[i] ^= scramble >> 16;
                break;
            case 2:
                data[i] ^= scramble >> 8;
                break;
            case 3:
                data[i] ^= scramble;
                scramble = scramble << 31 | scramble >> 1;
                break;
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
        m_328 = ox::io::CHelpIO::readInt(memFile) != 0;
    else
        m_328 = false;
    LevelRecordShown = false;
    MineralsRecordShown = false;

    if (header.Version > 28 && ox::io::CHelpIO::readByte(memFile))
    {
        LuaManager = new game::CLuaManager(Device, this);
        if (!LuaManager->initLuaBySaveFile(memFile, header.Version))
        {
            delete LuaManager;
            LuaManager = 0;
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
    float x = ViewPosition.X - 100.0f;
    float y = ViewPosition.Y - 100.0f;
    entity::gp_entityManager->update(0.001f,
        ox::core::CRect<float>(x, y, ScreenSize.Width + 200.0f + x, ScreenSize.Height + 200.0f + y));
    GameSpeed = 0;
    result = true;
    return result;
}

bool CPlayState::initializeNewGame()
{
    eraseGameObjects();
    StartTime = time(0);
    int random = ox::algo::CRand::rand();
    RandomValue = (random & 0xffffff) | ((random >> 8) + (random >> 16) + random) << 24;
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
    m_328 = false;
    if (GameMode != game::EGM_CAMPAIGN)
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
    else
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
    if (ScreenSize.Height - 600 > 40 && ScreenSize.Width - 800 > 40 && GuiSprites[GS_MINIMAP_BACKGROUND])
    {
        ShowMinimap = true;
        minimapSize = ScreenSize.Width - 800 > ScreenSize.Height - 600 ? ScreenSize.Height - 500 : ScreenSize.Width - 700;
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
    BarLeftArea = ox::core::CRect<int>(bar.UpperLeftCorner, bottomLeftSize);
    ox::core::CPosition2d<int> rightCorner(bar.LowerRightCorner.X - bottomRightSize.Width,
        bar.LowerRightCorner.Y - bottomRightSize.Height);
    BarRightArea = ox::core::CRect<int>(rightCorner, bottomRightSize);
    BarCenterArea = ox::core::CRect<int>(BarLeftArea.LowerRightCorner.X, BarLeftArea.UpperLeftCorner.Y,
        BarRightArea.UpperLeftCorner.X, BarLeftArea.LowerRightCorner.Y);

    int barWidth = bar.getWidth();
    int barHeight = bar.getHeight();
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

    float frameDelta = ox::core::clamp(GAME_SPEED_MULTIPLIERS[GameSpeed] * time, 0.0f, GameSpeed < 5 ? 0.06f : 0.5f);
    if (GameMode == game::EGM_WAVE)
    {
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
                sendCustomEvent((ECUSTOM_EVENT)21, 6);
                if (GameTime < 3600.0f)
                    sendCustomEvent((ECUSTOM_EVENT)21, 7);
            }
            else if ((int)previousTime / 300 != (int)GameTime / 300)
                game::gp_statistics->reportNewThreatLevel((int)GameTime / 300, GameTime);
        }
    }
    else if (GameMode == game::EGM_RUSH)
    {
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
                sendCustomEvent((ECUSTOM_EVENT)21, 4);
                sendCustomEvent((ECUSTOM_EVENT)21, 5);
                if (!m_328)
                    sendCustomEvent((ECUSTOM_EVENT)21, 13);
            }
            else if ((int)previousTime / 60 != (int)GameTime / 60)
                game::gp_statistics->reportNewThreatLevel((int)GameTime / 60, GameTime);
        }
    }
    else if (GameMode == game::EGM_CREATIVE)
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
    }
    else
        GameTime += frameDelta;

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
    if (m_764)
    {
        m_768 -= frameDelta;
        if (m_768 <= 0)
            m_764 = false;
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
            VisibleArea = ox::core::CRect<float>(ViewPosition.X - 100.0f, ViewPosition.Y - 100.0f,
                ScreenSize.Width + 200.0f + (ViewPosition.X - 100.0f), ScreenSize.Height + 200.0f + (ViewPosition.Y - 100.0f));
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
                if (Follow.Entity)
                {
                    float x = Follow.Entity->getPosition().X - ScreenSizeF.Width * 0.5f;
                    float y = Follow.Entity->getPosition().Y - Follow.Entity->getPosition().Z - ScreenSizeF.Height * 0.5f;
                    if (FollowJump)
                    {
                        ViewPosition.X = x;
                        ViewPosition.Y = y;
                        FollowJump = false;
                    }
                    else
                    {
                        ViewPosition.X += (x - ViewPosition.X) * 4.0f * step;
                        ViewPosition.Y += (y - ViewPosition.Y) * 4.0f * step;
                    }
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

    if (GameMode == game::EGM_RUSH)
    {
        if (game::gp_statistics->getRushModeDamage() > 0 && game::gp_statistics->getRushModeDamage() < 510.0f)
            sendCustomEvent((ECUSTOM_EVENT)22, 32);
    }
    else if (GameMode == game::EGM_INSANE)
    {
        if (ThreatLevel->getThreatLevel() == 50)
        {
            sendCustomEvent((ECUSTOM_EVENT)21, 9);
            sendCustomEvent((ECUSTOM_EVENT)21, 8);
        }
    }
    else if (GameMode == game::EGM_NORMAL)
    {
        if (game::gp_statistics->getGameStatValue(2) == 1)
            sendCustomEvent((ECUSTOM_EVENT)22, 26);
        if (ThreatLevel->getThreatLevel() == 11)
            sendCustomEvent((ECUSTOM_EVENT)22, 30);
        if (ThreatLevel->getThreatLevel() == 50)
            sendCustomEvent((ECUSTOM_EVENT)21, 1);
        if (ThreatLevel->getThreatLevel() == 100)
        {
            sendCustomEvent((ECUSTOM_EVENT)21, 2);
            sendCustomEvent((ECUSTOM_EVENT)21, 3);
        }
        if (ThreatLevel->getThreatLevel() == 15 && !m_328 && game::gp_world->getPlanet() == 0)
            sendCustomEvent((ECUSTOM_EVENT)21, 0);
        if (ThreatLevel->getThreatLevel() <= 10 && HarvestingCount >= 20)
            sendCustomEvent((ECUSTOM_EVENT)21, 17);
        if (HarvestingCount >= 50)
            sendCustomEvent((ECUSTOM_EVENT)21, 12);
        if (OverheatedCount >= 50)
            sendCustomEvent((ECUSTOM_EVENT)21, 18);
    }

    if (GameOver)
    {
        GameOverTime -= time;
        if (GameOverTime <= 0)
        {
            NextState = 3;
            if (GameMode != game::EGM_CAMPAIGN)
            {
                game::CHighscoreInfo* info;
                if (GameWon ? GameMode != game::EGM_CREATIVE : GameMode == game::EGM_NORMAL || GameMode == game::EGM_INSANE)
                    info = new game::CHighscoreInfo(PlayerName.c_str(), PlayerGroup.c_str(), RandomValue, StartTime,
                        GameMode, game::gp_world->getPlanet(), game::gp_statistics->getGameStatValue(1),
                        ThreatLevel->getThreatLevel(), GameTime);
                else
                    info = 0;
                game::CHighscoreInfo::setNewHighscoreInfo(info);
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

    if (Action == 2)
    {
        if (CursorMoved)
            updateRecycleBuilding();
    }
    else if (Action == 1 && CursorMoved)
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
    if (record == 0 || time < record)
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
            name = settings::gp_systemConfig->getLocalizedText(entity::ENTITY_KEY_NAMES[SelectedEntity->getEntityType()]);
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
        GuiElements[GUI_ID_UNLINK]->setVisible(tower || linker);
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

    sendCustomEvent((ECUSTOM_EVENT)22, 31);
    if (reward >= 16250)
        sendCustomEvent((ECUSTOM_EVENT)21, 19);
    if (!m_328)
        sendCustomEvent((ECUSTOM_EVENT)21, 23);
    if (GameTime < 900.0f)
        sendCustomEvent((ECUSTOM_EVENT)21, 22);
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
        message.Text = settings::gp_systemConfig->getLocalizedText(L"ingame:waveWarningMessage", aliens.c_str());
        addInfoLine(&message, false);
    }
    setWaveListToggle(!ListVisible[LIST_WAVES]);
    setWaveListToggle(!ListVisible[LIST_WAVES]);
    m_328 = false;
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
                    ox::gui::IGUILayout* popup = GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 50, 50),
                        GUIEnvironment->getRootGUIElement(), -1);
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
                ox::gui::IGUILayout* popup = GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 50, 50),
                    GUIEnvironment->getRootGUIElement(), -1);
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
    ox::io::CHelpIO::writeInt(file, m_328);
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
        header.ThreatLevel = ThreatLevel->getThreatLevel();
        header.Minerals = Minerals;
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
    m_111 = false;
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
            message.Text = settings::gp_systemConfig->getLocalizedText(L"ingame:welcomeTimeScores",
                ox::core::CStringFunctions::millisecondsToWide(time * 0.001f, true).c_str());
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
