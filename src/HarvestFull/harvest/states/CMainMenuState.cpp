// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include <math.h>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CAlienEntity.h"
#include "CMainMenuState.h"
#include "CLoadingScreen.h"
#include "harvest/CHarvestFullMain.h"
#include "harvest/ECustomEvents.h"
#include "harvest/game/CLuaManager.h"
#include "harvest/gfx/CScatterShader.h"
#include "harvest/gui/CAchievementsScreen.h"
#include "harvest/gui/CHighscoreScreen.h"
#include "harvest/gui/CMenuInfoDialog.h"
#include "harvest/gui/CProfileScreen.h"
#include "harvest/gui/CSaveGameScreen.h"
#include "harvest/gui/CSettingsScreen.h"
#include "harvest/gui/CStatisticsScreen.h"
#include "harvest/settings/CHarvestProfile.h"
#include "harvest/settings/CProfileManager.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/IOxDevice.h"
#include "ox/algo/CRegulator.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/core/CBasic.h"
#include "ox/core/CLine3d.h"
#include "ox/io/IFileSystem.h"
#include "ox/gui/IGUIButton.h"
#include "ox/gui/IGUICheckBox.h"
#include "ox/gui/IGUIElement.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIImage.h"
#include "ox/gui/IGUILayout.h"
#include "ox/gui/IGUIListBox.h"
#include "ox/gui/IGUISkin.h"
#include "ox/gui/IGUIStaticText.h"
#include "ox/scene/IAnimatedMeshSceneNode.h"
#include "ox/scene/IBillboardSceneNode.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/scene/ILightSceneNode.h"
#include "ox/scene/ISceneCollisionManager.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNodeAnimator.h"
#include "ox/video/IImage.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {
namespace game {

static const char* const PlanetDiffSpecTextures[] =
{
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/HephDiffSpec.tga",
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/PosDiffSpec.tga",
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/AresDiffSpec.tga"
};

static const char* const PlanetNormGlowTextures[] =
{
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/HephNormGlow.tga",
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/PosNormGlow.tga",
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/AresNormGlow.tga"
};

//! The planet textures without shaders.
static const char* const PlanetTextures[] =
{
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/HephNoShader.jpg",
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/PosNoShader.jpg",
    "$GAME_RESOURCES$/harvestClientData/gfx/shaders/AresNoShader.jpg"
};

} // end namespace game

namespace states {

static const char* const SKYBOX_TEXTURES[] =
{
    "$GAME_RESOURCES$/harvestClientData/gfx/skyboxRoof.jpg",
    "$GAME_RESOURCES$/harvestClientData/gfx/skyboxNorth.jpg",
    "$GAME_RESOURCES$/harvestClientData/gfx/skyboxWest.jpg",
    "$GAME_RESOURCES$/harvestClientData/gfx/skyboxEast.jpg",
    "$GAME_RESOURCES$/harvestClientData/gfx/skyboxSouth.jpg",
    "$GAME_RESOURCES$/harvestClientData/gfx/skyboxFloor.jpg"
};

static const wchar_t* const PLANET_NAMES[] = { L"Hephaestus", L"Poseidon", L"Ares" };
static const char* const PLANET_ICONS[] = { "PlanetIcon1", "PlanetIcon2", "PlanetIcon3" };
static const wchar_t* const PLANET_DESCRIPTIONS[] =
{
    L"planetdescription:hephaestus",
    L"planetdescription:poseidon",
    L"planetdescription:ares"
};

static const ox::core::CVector3d<float> SUN_POSITION(800.0f, 0.0f, 0.0f);
static const ox::core::CDimension2d<float> SUN_OUTER_FLARE_SIZE(500.0f, 500.0f);
static const ox::core::CDimension2d<float> SUN_INNER_FLARE_SIZE(20.0f, 20.0f);
static const ox::core::CVector3d<float> PLANET_POSITIONS[] =
{
    ox::core::CVector3d<float>(-250.0f, 0.0f, 0.0f),
    ox::core::CVector3d<float>(-200.0f, 20.0f, 80.0f),
    ox::core::CVector3d<float>(-150.0f, -10.0f, -30.0f)
};
static const ox::core::CVector3d<float> PLANET_GROUP_CENTER(-200.0f, 5.0f, 25.0f);
//! The camera circles the planets with these radii.
static const ox::core::CVector3d<float> PLANET_GROUP_RADIUS(100.0f, -10.0f, -90.0f);
static const ox::core::CVector3d<float> ATRUM_POSITION(-900.0f, -35.0f, 25.0f);
static const ox::core::CVector3d<float> PLANET_SELECT_POSITION(-200.0f, 40.0f, -100.0f);
static const ox::core::CVector3d<float> PLANET_SELECT_VIEW(-200.0f, 0.0f, 0.0f);

const wchar_t* GAME_MODE_DESCRIPTIONS[] =
{
    L"gamemodedescription:normal",
    L"gamemodedescription:wave",
    L"gamemodedescription:insane",
    L"gamemodedescription:rush",
    L"gamemodedescription:creative"
};

const char* GAME_MODE_ICONS[] =
{
    "ModeIconNormal",
    "ModeIconWave",
    "ModeIconInsane",
    "ModeIconRush",
    "ModeIconCreative"
};

const wchar_t* MODE_STATS[] =
{
    L"modestat:strategy",
    L"modestat:tactics",
    L"modestat:pressure",
    L"modestat:design"
};

const char* MODE_STRATEGY_ICONS[] =
{
    "ModeMeter2|right",
    "ModeMeter4|right",
    "ModeMeter1|right",
    "ModeMeter2|right",
    "ModeMeter1|right"
};

const char* MODE_TACTICS_ICONS[] =
{
    "ModeMeter2|right",
    "ModeMeter2|right",
    "ModeMeter4|right",
    "ModeMeter3|right",
    "ModeMeter1|right"
};

const char* MODE_PRESSURE_ICONS[] =
{
    "ModeMeter3|right",
    "ModeMeter2|right",
    "ModeMeter4|right",
    "ModeMeter4|right",
    "ModeMeter1|right"
};

const char* MODE_DESIGN_ICONS[] =
{
    "ModeMeter3|right",
    "ModeMeter3|right",
    "ModeMeter1|right",
    "ModeMeter2|right",
    "ModeMeter4|right"
};

//! The GUI element type of check boxes.
static const int CHECK_BOX_ELEMENT_TYPE = 6;
//! The GUI event of a mod check box.
static const ox::gui::EGUI_EVENT_TYPE MOD_CHECKBOX_EVENT = (ox::gui::EGUI_EVENT_TYPE)8;

CMainMenuState::CMainMenuState()
    : LoadingScreen(0), InitStep(0), BoldFont(0), SmallFont(0), MenuPackage(0), RootElement(0),
      UnusedElement(0), ButtonGroup(0), ModalWindow(0), ModList(0), SettingsScreen(0), HighscoreScreen(0),
      StatisticsScreen(0), SaveGameScreen(0), InfoDialog(0), ProfileScreen(0), AchievementsScreen(0),
      UnusedValue(0), SelectedPlanet(-1), HoveredGameMode(-1), Mode(MODE_NEUTRAL), PlanetHovered(false),
      Time(0.0f), ZoomTime(0.0f), FadeTime(0.0f), FadeAlpha(0.0f), SkyBox(0), Sun(0), PopupPlanet(0),
      GameModeWindow(0)
{
    for (int i = 0; i < 256; ++i)
        Keys[i] = false;
    for (int i = 0; i < PLANET_COUNT * 2; ++i)
        Shaders[i] = 0;
    for (int i = 0; i < SPRITE_COUNT; ++i)
        Sprites[i] = 0;
}

CMainMenuState::~CMainMenuState()
{
    if (AudioDriver)
        AudioDriver->stopAllMusic();
    if (LoadingScreen)
        delete LoadingScreen;
    if (RootElement)
        RootElement->remove();

    for (int i = 0; i < PLANET_COUNT * 2; ++i)
        if (Shaders[i])
            Shaders[i]->drop();
    for (int i = 0; i < SPRITE_COUNT; ++i)
        if (Sprites[i])
            Sprites[i]->remove();

    if (SkyBox)
        SkyBox->remove();
    if (SceneManager)
        SceneManager->clear();

    if (SettingsScreen)
        delete SettingsScreen;
    if (HighscoreScreen)
        delete HighscoreScreen;
    if (StatisticsScreen)
        delete StatisticsScreen;
    if (SaveGameScreen)
        delete SaveGameScreen;
    if (InfoDialog)
        delete InfoDialog;
    if (ProfileScreen)
        delete ProfileScreen;
    if (AchievementsScreen)
        delete AchievementsScreen;

    if (Driver)
    {
        for (int i = 0; i < 6; ++i)
            Driver->removeTexture(SKYBOX_TEXTURES[i]);
        for (int i = 0; i < PLANET_COUNT; ++i)
        {
            Driver->removeTexture(game::PlanetDiffSpecTextures[i]);
            Driver->removeTexture(game::PlanetNormGlowTextures[i]);
        }
    }
}

int CMainMenuState::firstInit(ox::IOxDevice* device)
{
    if (CGameState::firstInit(device) == 1)
        return 1;

    LoadingScreen = new CLoadingScreen();
    return LoadingScreen->init(Driver) == 1;
}

int CMainMenuState::secondInit()
{
    switch (InitStep++)
    {
    case 0:
        Driver->setTextureCreationFlag((ox::video::E_TEXTURE_CREATION_FLAG)4, true);
        BoldFont = GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt");
        if (!BoldFont)
            return 1;
        GUIEnvironment->getSkin()->setFont(BoldFont);
        SmallFont = GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/smallFont.fnt");
        return 2;
    case 1:
        MenuPackage = Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true);
        if (!MenuPackage)
            return 1;
        GUIEnvironment->getSkin()->setSpritePackage(MenuPackage);
        return 2;
    case 2:
        GUIEnvironment->getSkin()->setColor((ox::gui::EGUI_DEFAULT_COLOR)8, ox::video::SColor(0xffffffff));
        return 2;
    case 3:
        ScreenSize = Driver->getScreenSize();
        RootElement = GUIEnvironment->addLayoutGroup(
            ox::core::CRect<int>(0, 0, ScreenSize.Width, ScreenSize.Height), GUIEnvironment->getRootGUIElement());
        HeadingText = GUIEnvironment->addStaticText(L"", ox::core::CRect<int>(0, 20, ScreenSize.Width, 60), false,
            false, RootElement, -1, L"");
        HeadingText->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_CENTER);
        return 2;
    case 4:
        {
            ButtonGroup = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 500, 100), RootElement);
            ButtonGroup->setID(ID_BUTTON_GROUP);
            ButtonGroup->setReportOnDraw(true);

            ox::gui::IGUIElement* group = ButtonGroup;
            Buttons[BUTTON_NEW_GAME] =
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), group, ID_NEW_GAME, 0);
            Buttons[BUTTON_LOAD_GAME] =
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), group, ID_LOAD_GAME, 0);
            Buttons[BUTTON_AWARDS] =
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), group, ID_AWARDS, 0);
            Buttons[BUTTON_HIGHSCORES] =
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), group, ID_HIGHSCORES, 0);
            Buttons[BUTTON_SETTINGS] =
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), group, ID_SETTINGS, 0);
            Buttons[BUTTON_PROFILE] =
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), group, ID_PROFILE, 0);
            Buttons[BUTTON_EXIT] =
                GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), group, ID_EXIT, 0);
        }
        return 2;
    case 6:
        SettingsScreen = new gui::CSettingsScreen(Device, true);
        return 2;
    case 7:
        HighscoreScreen = new gui::CHighscoreScreen(Device);
        StatisticsScreen = new gui::CStatisticsScreen(Device);
        SaveGameScreen = new gui::CSaveGameScreen(Device);
        InfoDialog = new gui::CMenuInfoDialog(Device);
        ProfileScreen = new gui::CProfileScreen(Device);
        AchievementsScreen = new gui::CAchievementsScreen(Device);
        return 2;
    case 8:
        {
            ox::core::CString<char> resources(Device->getFileSystem()->getDirectoryFromAlias("$GAME_RESOURCES$"));
            ox::core::CString<char> atmospherePath = resources;
            atmospherePath += "/harvestClientData/gfx/atmoSphere_48.obj";

            ox::scene::IAnimatedMesh* planetMesh =
                SceneManager->getMesh("$GAME_RESOURCES$/harvestClientData/gfx/planetSphere_24.obj");
            ox::scene::IAnimatedMesh* atmosphereMesh = SceneManager->getMesh(atmospherePath.c_str());
            if (!atmosphereMesh || !planetMesh)
                break;

            int shaderLevel = settings::gp_systemConfig->getShaderLevel();
            for (int i = 0; i < PLANET_COUNT; ++i)
            {
                PlanetNodes[i * 2] = SceneManager->addAnimatedMeshSceneNode(planetMesh, 0, ID_FIRST_PLANET + i);
                if (!PlanetNodes[i * 2])
                    continue;

                if (shaderLevel == settings::ESL_NONE)
                    PlanetNodes[i * 2]->setMaterialTexture(0, Driver->getTexture(game::PlanetTextures[i]));
                else
                {
                    PlanetNodes[i * 2]->setMaterialTexture(0, Driver->getTexture(game::PlanetDiffSpecTextures[i]));
                    PlanetNodes[i * 2]->setMaterialTexture(1, Driver->getTexture(game::PlanetNormGlowTextures[i]));
                    if (shaderLevel == settings::ESL_HIGH)
                    {
                        PlanetNodes[i * 2 + 1] = SceneManager->addAnimatedMeshSceneNode(atmosphereMesh,
                            PlanetNodes[i * 2], ID_FIRST_PLANET + i);
                        PlanetNodes[i * 2 + 1]->setMaterialType(ox::video::EMT_TRANSPARENT_ADD_COLOR);
                        PlanetNodes[i * 2 + 1]->setVisible(false);
                    }
                }
                PlanetNodes[i * 2]->setMaterialType(ox::video::EMT_SOLID);
                PlanetNodes[i * 2]->setPosition(PLANET_POSITIONS[i]);

                ox::scene::ISceneNodeAnimator* animator = SceneManager->createRotationAnimator(
                    ox::core::CVector3d<float>(0.0f, (i + 5) * -0.001f, 0.0f));
                PlanetNodes[i * 2]->addAnimator(animator);
                animator->drop();
            }

            ox::scene::IAnimatedMeshSceneNode* atrum =
                SceneManager->addAnimatedMeshSceneNode(planetMesh, 0, ID_ATRUM);
            ox::video::ITexture* atrumTexture =
                Driver->getTexture("$GAME_RESOURCES$/harvestClientData/gfx/skyboxRoof.jpg");
            if (!atrumTexture)
            {
                ox::video::IImage* image = Driver->createImageFromData((ox::video::ECOLOR_FORMAT)1,
                    ox::core::CDimension2d<int>(64, 64), 0);
                if (image)
                {
                    short* data = (short*)image->lock();
                    for (int i = 0; i < image->getImageDataSizeInPixels(); ++i)
                        data[i] = 0;
                    image->unlock();
                    atrumTexture = Driver->addTexture("AtrumBlack", image);
                    image->drop();
                }
            }
            atrum->setMaterialTexture(0, atrumTexture);
            atrum->setMaterialType(ox::video::EMT_SOLID);
            atrum->setPosition(ATRUM_POSITION);
        }
        return 2;
    case 9:
        Camera = SceneManager->addCameraSceneNode();
        SceneManager->setActiveCamera(Camera);
        CameraPositionTarget = PLANET_SELECT_POSITION;
        CameraPosition = ox::core::CVector3d<float>(-200.0f, -20.0f, -150.0f);
        CameraTarget = ox::core::CVector3d<float>(-350.0f, 0.0f, -70.0f);
        CameraTargetSpeed = ox::core::CVector3d<float>(-20.0f, 0.0f, 0.0f);
        CameraTargetTarget = PLANET_SELECT_VIEW;
        Camera->setPosition(CameraPosition);
        Camera->setTarget(CameraTarget);
        FieldOfView = Camera->getFOV();
        return 2;
    case 10:
        {
            Driver->setAmbientLight(ox::video::SColorf(ox::video::SColor(0, 32, 32, 32)));
            ox::scene::ILightSceneNode* light = SceneManager->addLightSceneNode(0, SUN_POSITION,
                ox::video::SColorf(1.0f, 1.0f, 1.0f, 1.0f), 100.0f);
            light->getLightData().Radius = 7500.0f;
            light->getLightData().DiffuseColor = ox::video::SColorf(ox::video::SColor(0, 117, 117, 117));

            int shaderLevel = settings::gp_systemConfig->getShaderLevel();
            if (shaderLevel != settings::ESL_NONE)
                for (int i = 0; i < PLANET_COUNT * 2; ++i)
                {
                    // Low shaders leave the atmospheres out.
                    if (shaderLevel == settings::ESL_LOW && i % 2)
                        continue;
                    Shaders[i] = new gfx::CScatterShader(Driver, Camera, PlanetNodes[i]);
                    if (i % 2)
                        Shaders[i]->initAtmo();
                    else
                        Shaders[i]->initGround(shaderLevel == settings::ESL_HIGH);
                }
        }
        return 2;
    case 11:
        {
            ox::video::ITexture* top = Driver->getTexture(SKYBOX_TEXTURES[0]);
            ox::video::ITexture* north = Driver->getTexture(SKYBOX_TEXTURES[1]);
            ox::video::ITexture* west = Driver->getTexture(SKYBOX_TEXTURES[2]);
            ox::video::ITexture* east = Driver->getTexture(SKYBOX_TEXTURES[3]);
            ox::video::ITexture* south = Driver->getTexture(SKYBOX_TEXTURES[4]);
            ox::video::ITexture* bottom = Driver->getTexture(SKYBOX_TEXTURES[5]);
            SkyBox = SceneManager->addSkyBoxSceneNode(top, bottom, east, west, south, north);
        }
        return 2;
    case 12:
        Sun = SceneManager->addBillboardSceneNode(0, SUN_OUTER_FLARE_SIZE, SUN_POSITION);
        Sun->setMaterialType(ox::video::EMT_TRANSPARENT_ADD_COLOR);
        Sun->getMaterial(0).Lighting = false;
        Sun->getMaterial(0).ZBuffer = false;
        Sun->setMaterialTexture(0, Driver->getTexture("$GAME_RESOURCES$/harvestClientData/gfx/particlewhite.jpg"));
        return 2;
    case 17:
        {
            Sprites[SPRITE_CONSOLE] = MenuPackage->addNewAnimationState("ConsolMain");

            ox::core::CString<wchar_t> language = settings::gp_systemConfig->getCurrentLanguageTag();
            if (language == L"en")
                Sprites[SPRITE_CONSOLE_TEXT] = MenuPackage->addNewAnimationState("ConsolTextEng");
            else
            {
                ox::core::CString<char> name("ConsolText_");
                name += ox::core::CString<char>(language.c_str());
                Sprites[SPRITE_CONSOLE_TEXT] = MenuPackage->addNewAnimationState(name.c_str());
                if (!Sprites[SPRITE_CONSOLE_TEXT])
                    Sprites[SPRITE_CONSOLE_TEXT] = MenuPackage->addNewAnimationState("ConsolTextEng");
            }
            Sprites[SPRITE_CONSOLE_TILE] = MenuPackage->addNewAnimationState("ConsolTile");

            Buttons[BUTTON_PROFILE]->setAnimations(MenuPackage, "CBtnProfile", true);
            Buttons[BUTTON_SETTINGS]->setAnimations(MenuPackage, "CBtnSettings", true);
            Buttons[BUTTON_HIGHSCORES]->setAnimations(MenuPackage, "CBtnHigh", true);
            Buttons[BUTTON_NEW_GAME]->setAnimations(MenuPackage, "CBtnNew", true);
            Buttons[BUTTON_LOAD_GAME]->setAnimations(MenuPackage, "CBtnLoad", true);
            Buttons[BUTTON_AWARDS]->setAnimations(MenuPackage, "CBtnAwards", true);
            Buttons[BUTTON_EXIT]->setAnimations(MenuPackage, "CBtnExit", true);
            realignGui();
        }
        return 2;
    case 18:
        Sprites[SPRITE_MODE_NORMAL] = MenuPackage->addNewAnimationState("ModeBtnSmallNormal");
        Sprites[SPRITE_MODE_WAVE] = MenuPackage->addNewAnimationState("ModeBtnSmallWave");
        Sprites[SPRITE_MODE_INSANE] = MenuPackage->addNewAnimationState("ModeBtnSmallInsane");
        Sprites[SPRITE_MODE_RUSH] = MenuPackage->addNewAnimationState("ModeBtnSmallRush");
        Sprites[SPRITE_MODE_CREATIVE] = MenuPackage->addNewAnimationState("ModeBtnSmallCreative");
        Sprites[SPRITE_MODE_SELECTOR] = MenuPackage->addNewAnimationState("ModeBtnSmallSelector");
        Sprites[SPRITE_LOGO] = MenuPackage->addNewAnimationState("HarvestLogo");
        return 2;
    case 19:
        game::CLuaManager::refreshAvailableLuaMods(Device);
        return 2;
    case 35:
        if (AudioDriver)
        {
            AudioDriver->loadMusic("ambience1.ogg");
            AudioDriver->loadMusic("ambience2.ogg");
            AudioDriver->loadMusic("ambience3.ogg");
        }
        for (int i = 0; i < PLANET_COUNT; ++i)
            PlanetDistances[i] = (PLANET_POSITIONS[i] - CameraPosition).getLength();
        return 2;
    case 36:
        if (g_scenarioResult == 1)
        {
            if (g_scenarioResultGameMode == 5)
                InfoDialog->setVisible(true, gui::CMenuInfoDialog::EIM_DEBRIEFING);
        }
        else if (g_scenarioResult == 3 || g_scenarioResult == 4)
            StatisticsScreen->setVisible(true);
        return 2;
    case 37:
        if (!settings::gp_profileManager->getCurrentProfile())
        {
            if (settings::gp_profileManager->getProfileNames(true).size() == 0)
                createEditProfileWindow(true);
            else
                createProfileListWindow();
        }
        return 2;
    // Idle steps, which give the loading screen time to show.
    case 5: case 13: case 14: case 15: case 16: case 20: case 21: case 22: case 23: case 24: case 25:
    case 26: case 27: case 28: case 29: case 30: case 31: case 32: case 33: case 34:
        return 2;
    }

    Device->setEventReceiver(this);
    NextState = 0;
    return 0;
}

void CMainMenuState::realignGui()
{
    ScreenSize = Driver->getScreenSize();
    if (RootElement)
        RootElement->setRelativePosition(ox::core::CRect<int>(0, 0, ScreenSize.Width, ScreenSize.Height));
    if (HeadingText)
        HeadingText->setRelativePosition(ox::core::CRect<int>(0, 20, ScreenSize.Width, 60));

    if (!ButtonGroup)
        return;

    if (Sprites[SPRITE_CONSOLE])
    {
        ox::core::CPosition2d<int> size = Sprites[SPRITE_CONSOLE]->getFrameSize(0);
        ox::core::CPosition2d<int> position((ScreenSize.Width - size.X) / 2, ScreenSize.Height - 150);
        ButtonGroupRect = ox::core::CRect<int>(position, position + size);
        ButtonGroup->setRelativePosition(ButtonGroupRect);
    }

    Buttons[BUTTON_PROFILE]->moveTo(ox::core::CPosition2d<int>(128, 36));
    Buttons[BUTTON_SETTINGS]->moveTo(ox::core::CPosition2d<int>(210, 28));
    Buttons[BUTTON_HIGHSCORES]->moveTo(ox::core::CPosition2d<int>(312, 23));
    Buttons[BUTTON_NEW_GAME]->moveTo(ox::core::CPosition2d<int>(423, 23));
    Buttons[BUTTON_LOAD_GAME]->moveTo(ox::core::CPosition2d<int>(528, 24));
    Buttons[BUTTON_AWARDS]->moveTo(ox::core::CPosition2d<int>(633, 27));
    Buttons[BUTTON_EXIT]->moveTo(ox::core::CPosition2d<int>(726, 36));
}

void CMainMenuState::createEditProfileWindow(bool firstProfile)
{
    if (ProfileScreen)
        ProfileScreen->createEditProfileWindow(firstProfile);
}

void CMainMenuState::createProfileListWindow()
{
    if (ProfileScreen)
        ProfileScreen->createProfileListWindow();
}

int CMainMenuState::updateState(float time)
{
    if (!Device->run() || !Driver)
        return 1;

    float frameDelta = ox::core::clamp(time, 0.0f, 0.06f);
    if (Camera)
    {
        Time += frameDelta;
        if (Mode == MODE_NEUTRAL)
        {
            // Circle the planets.
            CameraTargetTarget = PLANET_GROUP_CENTER;
            float angle = Time * 3.1415927f * 0.005f;
            CameraPositionTarget = PLANET_GROUP_CENTER;
            CameraPositionTarget.X += PLANET_GROUP_RADIUS.X * cos(angle + 1.5707964f);
            CameraPositionTarget.Y += PLANET_GROUP_RADIUS.Y * cos(angle * 3.1f);
            CameraPositionTarget.Z += PLANET_GROUP_RADIUS.Z * sin(angle + 1.5707964f);
        }
        else if (Mode == MODE_START_GAME)
        {
            if (CameraTarget.getDistanceFrom(CameraTargetTarget) > 3.0)
                ox::algo::C3dRegulator::fakeSpeedRegulation(CameraTargetSpeed, CameraTarget, CameraTargetTarget,
                    30.0f, 0.8f, frameDelta);

            if (ZoomTime < 3.0f)
            {
                FieldOfView = 3.1415927f / (ZoomTime / -3.0f + 2.5f);
                ZoomTime += frameDelta;
            }

            if (FadeTime < 4.0f)
            {
                FadeAlpha = 0.25f * FadeTime * 255.0f;
                FadeTime += frameDelta;
            }
            else
                NextState = EGS_PLAY;
        }

        if (Mode != MODE_START_GAME)
            ox::algo::C3dRegulator::fakeSpeedRegulation(CameraTargetSpeed, CameraTarget, CameraTargetTarget, 30.0f,
                0.5f, frameDelta);

        // Fly over the planets in the way.
        ox::core::CVector3d<float> target = CameraPositionTarget;
        ox::core::CLine3d<float> line(CameraPosition, CameraPositionTarget);
        for (int i = 0; i < PLANET_COUNT; ++i)
        {
            ox::core::CVector3d<float> planetPosition = PlanetNodes[i * 2]->getAbsolutePosition();
            double distance = planetPosition.getDistanceFromSQ(line.getClosestPoint(planetPosition));
            if (distance < 121.0)
            {
                double offset = sqrt(distance) * 10.0;
                if (planetPosition.Y > target.Y)
                    target.Y = CameraPosition.Y - (float)offset;
                else
                    target.Y = CameraPosition.Y + (float)offset;
            }
        }
        ox::algo::C3dRegulator::fakeSpeedRegulation(CameraSpeed, CameraPosition, target, 20.0f, 0.5f, frameDelta);

        Camera->setPosition(CameraPosition);
        Camera->setTarget(CameraTarget);
        Camera->setFOV(FieldOfView);

        if (!Sun->getAutomaticCulling())
        {
            // Hide the sun behind the planets.
            ox::core::CLine3d<float> sunLine(Camera->getAbsolutePosition(), Sun->getAbsolutePosition());
            Sun->setVisible(true);
            Sun->setSize(SUN_OUTER_FLARE_SIZE);
            for (int i = 0; i < PLANET_COUNT; ++i)
            {
                ox::core::CVector3d<float> planetPosition = PlanetNodes[i * 2]->getAbsolutePosition();
                double distance = planetPosition.getDistanceFromSQ(sunLine.getClosestPoint(planetPosition));
                if (distance < 121.0)
                {
                    if (distance < 100.0)
                    {
                        Sun->setVisible(false);
                        break;
                    }
                    float scale = 1.0 - (11.0 - sqrt(distance));
                    Sun->setSize(ox::core::CDimension2d<float>(SUN_OUTER_FLARE_SIZE.Width * scale,
                        SUN_OUTER_FLARE_SIZE.Height * scale));
                }
            }
        }

        const char* ambience[PLANET_COUNT] = { "ambience1.ogg", "ambience2.ogg", "ambience3.ogg" };
        for (int i = 0; i < PLANET_COUNT; ++i)
        {
            PlanetDistances[i] = (PLANET_POSITIONS[i] - CameraPosition).getLength();
            if (AudioDriver)
            {
                if (PlanetDistances[i] < 100.0f)
                {
                    float volume = PlanetDistances[i] / -100.0f + 1.0f;
                    if (!AudioDriver->updateMusic(ambience[i], volume))
                        AudioDriver->playMusic(ambience[i], volume, true);
                }
                else
                    AudioDriver->stopMusic(ambience[i]);
            }
        }
    }

    if (PopupPlanet && allowPlanetSelection())
    {
        ox::scene::ISceneNode* planet = SceneManager->getSceneNodeFromId(ID_FIRST_PLANET + SelectedPlanet);
        updatePopupPlanet(false, SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(
            planet->getAbsolutePosition()));
    }

    if (HighscoreScreen && HighscoreScreen->isVisible())
        HighscoreScreen->update(time);
    if (StatisticsScreen && StatisticsScreen->isVisible())
        StatisticsScreen->update(time);
    if (AchievementsScreen && AchievementsScreen->isVisible())
        AchievementsScreen->update(time);
    if (InfoDialog && InfoDialog->isVisible())
        InfoDialog->update(time);

    InitStep = 40;
    return NextState;
}

bool CMainMenuState::allowPlanetSelection()
{
    if (Mode == MODE_PLANET_SELECT)
        return true;
    if (Mode != MODE_NEUTRAL)
        return false;

    if (HighscoreScreen && HighscoreScreen->isVisible())
        return false;
    if (StatisticsScreen && StatisticsScreen->isVisible())
        return false;
    if (SaveGameScreen && SaveGameScreen->isVisible())
        return false;
    if (InfoDialog && InfoDialog->isVisible())
        return false;
    if (ProfileScreen && ProfileScreen->isVisible())
        return false;
    if (SettingsScreen)
        return !SettingsScreen->isVisible();
    return true;
}

void CMainMenuState::fillLayoutWithPlanetInfo(ox::gui::IGUILayout* layout, int planet, bool popup)
{

    ox::gui::IGUIStaticText* name = GUIEnvironment->addStaticText(PLANET_NAMES[planet],
        ox::core::CRect<int>(0, 0, 200, 30), false, true, layout, -1, L"");
    name->setOverrideFont(GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/largeFont.fnt"));
    name->setParagraphIcon(PLANET_ICONS[planet],
        Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat", true), true);
    if (popup)
        name->activateProgressiveReveal(0);

    ox::core::CString<wchar_t> text = settings::gp_systemConfig->getLocalizedText(PLANET_DESCRIPTIONS[planet]);
    ox::gui::IGUIStaticText* description =
        GUIEnvironment->addStaticText(text.c_str(), 200, layout, SmallFont, -1, L"");
    description->LayoutFlags = "br";
    if (popup)
        description->activateProgressiveReveal(0);

    if (planet > 0)
    {
        text = settings::gp_systemConfig->getLocalizedText(L"planetdescription:experienced");
        ox::gui::IGUIStaticText* experienced =
            GUIEnvironment->addStaticText(text.c_str(), 200, layout, SmallFont, -1, L"");
        experienced->setOverrideColor(ox::video::SColor(0xffffa8a8));
        experienced->LayoutFlags = "br";
        if (popup)
            experienced->activateProgressiveReveal(0);
    }

    layout->sortRiver(true, 5, 5, false);

    if (popup && AudioDriver)
    {
        const char* hoverSounds[] = { "PlanetHover1.ogg", "PlanetHover2.ogg", "PlanetHover3.ogg" };
        AudioDriver->playVoice(hoverSounds[planet]);
    }
}

void CMainMenuState::updatePopupPlanet(bool recreate, const ox::core::CPosition2d<int>& position)
{
    if (PopupPlanet && recreate)
    {
        PopupPlanet->remove();
        PopupPlanet = 0;
    }

    if (!PopupPlanet)
    {
        ox::gui::IGUILayout* popup = GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 200, 200), RootElement, -1);
        popup->setAnimations(MenuPackage, "Black");
        PopupPlanet = popup;
        fillLayoutWithPlanetInfo(popup, SelectedPlanet, true);
    }

    ox::core::CRect<int> rect = PopupPlanet->getRelativePosition();
    PopupPlanet->moveTo(position - ox::core::CPosition2d<int>(rect.getWidth() / 2, rect.getHeight()));
    PopupPlanet->setVisible(true);
}

bool CMainMenuState::OnEvent(const ox::event::SEvent& event)
{
    if (InitStep < 40)
        return false;

    if (ProfileScreen && ProfileScreen->isVisible() && ProfileScreen->OnEvent(event))
        return true;
    if (HighscoreScreen && HighscoreScreen->isVisible() && HighscoreScreen->OnEvent(event))
        return true;
    if (StatisticsScreen && StatisticsScreen->isVisible() && StatisticsScreen->OnEvent(event))
        return true;
    if (SaveGameScreen && SaveGameScreen->isVisible() && SaveGameScreen->OnEvent(event))
        return true;
    if (InfoDialog && InfoDialog->isVisible() && InfoDialog->OnEvent(event))
        return true;
    if (AchievementsScreen && AchievementsScreen->isVisible() && AchievementsScreen->OnEvent(event))
        return true;

    if (ModalWindow && event.EventType == ox::event::EET_MOUSE_INPUT_EVENT)
    {
        // Clicking a mod's row toggles its check box.
        if (ModList && HoveredGameMode == 4 && event.MouseInput.Event == ox::event::EMIE_LMOUSE_PRESSED_DOWN)
        {
            ox::core::CPosition2d<int> mouse(event.MouseInput.X, event.MouseInput.Y);
            if (ModList->getAbsolutePosition().isPointInside(mouse))
            {
                int count = ModList->getItemCount();
                for (int i = 0; i < count; ++i)
                {
                    ox::gui::IGUIElement* item = ModList->getListItem(i);
                    if (!item || !item->getAbsolutePosition().isPointInside(mouse))
                        continue;

                    ox::gui::IGUIElement* box = item->getElementFromId(item->getID(), true);
                    if (!box || box->getType() != CHECK_BOX_ELEMENT_TYPE)
                        continue;

                    ox::gui::IGUICheckBox* checkBox = (ox::gui::IGUICheckBox*)box;
                    checkBox->setChecked(!checkBox->isChecked());
                    int mod = item->getID() - ID_FIRST_MOD;
                    if (mod >= 0 && mod < (int)game::CLuaManager::s_availableLuaScriptFiles.size())
                        game::CLuaManager::s_availableLuaScriptFiles[mod].Enabled = checkBox->isChecked();
                    return true;
                }
            }
        }
        return false;
    }

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
                case ID_BACK:
                    enterPlanetSelectMode();
                    return true;
                case ID_LOAD_GAME:
                    enterNeutralMode();
                    SaveGameScreen->setVisible(true, true);
                    return true;
                case ID_SETTINGS:
                    enterNeutralMode();
                    SettingsScreen->setVisible(true);
                    return true;
                case ID_PROFILE:
                    enterNeutralMode();
                    createProfileListWindow();
                    return true;
                case ID_HIGHSCORES:
                    enterNeutralMode();
                    HighscoreScreen->setVisible(true);
                    return true;
                case ID_AWARDS:
                    enterNeutralMode();
                    AchievementsScreen->setVisible(true);
                    return true;
                case ID_EXIT:
                    NextState = EGS_QUIT;
                    return true;
                case ID_MODS_OK:
                    if (ModalWindow)
                    {
                        ModalWindow->remove();
                        ModalWindow = 0;
                        ModList = 0;
                    }
                    Mode = MODE_START_GAME;
                    CameraTargetTarget =
                        SceneManager->getSceneNodeFromId(ID_FIRST_PLANET + SelectedPlanet)->getAbsolutePosition();
                    if (GameModeWindow)
                        GameModeWindow->setVisible(false);
                    hidePopupPlanet();
                    if (HeadingText)
                        HeadingText->setText(ox::core::CString<wchar_t>().c_str());
                    return true;
                case ID_MODS_CANCEL:
                    if (ModalWindow)
                    {
                        ModalWindow->remove();
                        ModalWindow = 0;
                        ModList = 0;
                    }
                    return true;
                }
                break;
            case MOD_CHECKBOX_EVENT:
                {
                    int mod = id - ID_FIRST_MOD;
                    if (mod >= 0 && mod < (int)game::CLuaManager::s_availableLuaScriptFiles.size())
                        game::CLuaManager::s_availableLuaScriptFiles[mod].Enabled =
                            ((ox::gui::IGUICheckBox*)event.GUIEvent.Caller)->isChecked();
                }
                break;
            case ox::gui::EGET_MESSAGEBOX_YES:
                if (id == ID_SHUTTLE_RACE_BOX)
                {
                    sendCustomEvent(ECE_START_SHUTTLE_RACE);
                    return true;
                }
                break;
            case ox::gui::EGET_MESSAGEBOX_NO:
                if (id == ID_SHUTTLE_RACE_BOX)
                {
                    enterNeutralMode();
                    return true;
                }
                break;
            case ox::gui::EGET_ELEMENT_DRAWN:
                switch (id)
                {
                case ID_FIRST_GAME_MODE:
                case ID_FIRST_GAME_MODE + 1:
                case ID_FIRST_GAME_MODE + 2:
                case ID_FIRST_GAME_MODE + 3:
                case ID_FIRST_GAME_MODE + 4:
                    {
                        ox::video::ISpriteAnimationState* sprite =
                            Sprites[id - ID_FIRST_GAME_MODE + SPRITE_MODE_NORMAL];
                        ox::core::CRect<int> rect = event.GUIEvent.Caller->getAbsolutePosition();
                        if (sprite)
                        {
                            ox::core::CPosition2d<int> size = sprite->getFrameSize(0);
                            sprite->draw(ox::core::CPosition2d<int>(
                                rect.UpperLeftCorner.X + (rect.getWidth() - size.X) / 2,
                                rect.UpperLeftCorner.Y + (rect.getHeight() - size.Y) / 2), 0,
                                ox::video::SColor(0xffffffff));
                        }
                    }
                    return true;
                case ID_BUTTON_GROUP:
                    if (Mode == MODE_GAME_MODE_SELECT && HoveredGameMode >= 0 && GameModeButtons[HoveredGameMode] &&
                        Sprites[SPRITE_MODE_SELECTOR])
                    {
                        ox::core::CRect<int> rect = GameModeButtons[HoveredGameMode]->getAbsolutePosition();
                        Sprites[SPRITE_MODE_SELECTOR]->draw(ox::core::CPosition2d<int>(
                            (rect.UpperLeftCorner.X + rect.LowerRightCorner.X) / 2,
                            (rect.UpperLeftCorner.Y + rect.LowerRightCorner.Y) / 2), 0, ox::video::SColor(0xffffffff));
                    }
                    if (Sprites[SPRITE_CONSOLE_TILE])
                    {
                        ox::core::CPosition2d<int> size = Sprites[SPRITE_CONSOLE_TILE]->getFrameSize(0);
                        for (int x = 0; x < ScreenSize.Width; x += size.X)
                            Sprites[SPRITE_CONSOLE_TILE]->draw(ox::core::CPosition2d<int>(x,
                                ButtonGroup->getAbsolutePosition().LowerRightCorner.Y - size.Y), 0,
                                ox::video::SColor(0xffffffff));
                    }
                    if (Sprites[SPRITE_CONSOLE])
                        Sprites[SPRITE_CONSOLE]->draw(ButtonGroup->getAbsolutePosition().UpperLeftCorner, 0,
                            ox::video::SColor(0xffffffff));
                    if (Sprites[SPRITE_CONSOLE_TEXT])
                        Sprites[SPRITE_CONSOLE_TEXT]->draw(ButtonGroup->getAbsolutePosition().UpperLeftCorner +
                            ox::core::CPosition2d<int>(143, 25), 0, ox::video::SColor(0xffffffff));
                    return true;
                case ID_GAME_MODE_INFO:
                case ID_SHADED_PANEL:
                    Driver->draw2DRectangle(ox::video::SColor(0x80000000),
                        event.GUIEvent.Caller->getAbsolutePosition(), 0);
                    return true;
                }
                break;
            }
        }
        break;
    case ox::event::EET_MOUSE_INPUT_EVENT:
        {
            ox::core::CPosition2d<int> mouse(event.MouseInput.X, event.MouseInput.Y);
            if (event.MouseInput.Event == ox::event::EMIE_LMOUSE_PRESSED_DOWN)
            {
                if (PlanetHovered && allowPlanetSelection())
                {
                    ox::scene::ISceneNode* planet = SceneManager->getSceneNodeFromId(ID_FIRST_PLANET + SelectedPlanet);
                    if (!planet)
                        return false;

                    switch (SelectedPlanet)
                    {
                    case 0:
                    case 1:
                    case 2:
                        {
                            int score = settings::gp_profileManager->getCurrentProfile()->getAchievementScore();
                            if ((score < 46 && SelectedPlanet == 1) || (score < 84 && SelectedPlanet == 2))
                            {
                                AudioDriver->playSound("BtnDenial.ogg", 1.0f, 0.0f, 1.0f);
                                createLockedPlanetMessageBox(SelectedPlanet);
                                return false;
                            }
                            enterGameModeSelectMode(SelectedPlanet);
                        }
                        break;
                    case 3:
                        PlanetHovered = false;
                        Mode = MODE_GAME_MODE_SELECT;
                        GUIEnvironment->addMessageBox(L"", L"Would you like to start the WICKED AWESOME GAME?", true,
                            ox::gui::EMBF_YES | ox::gui::EMBF_NO, 0, ID_SHUTTLE_RACE_BOX);
                        break;
                    }

                    CameraPositionTarget = planet->getAbsolutePosition() + ox::core::CVector3d<float>(13.0f, 0.0f, -10.0f);
                    CameraTargetTarget = planet->getAbsolutePosition() + ox::core::CVector3d<float>(13.0f, 0.0f, 0.0f);
                }
                else if (HoveredGameMode >= 0 && Mode == MODE_GAME_MODE_SELECT && GameModeButtons[HoveredGameMode] &&
                    GameModeButtons[HoveredGameMode]->getAbsolutePosition().isPointInside(mouse))
                {
                    g_gameMode = HoveredGameMode;
                    g_gamePlanet = SelectedPlanet;
                    switch (HoveredGameMode)
                    {
                    case 0:
                        AudioDriver->playVoice("mode_normal.ogg");
                        break;
                    case 1:
                        AudioDriver->playVoice("mode_wave.ogg");
                        break;
                    case 2:
                        AudioDriver->playVoice("mode_insane.ogg");
                        break;
                    case 3:
                        AudioDriver->playVoice("mode_rush.ogg");
                        break;
                    case 4:
                        AudioDriver->playVoice("mode_creative.ogg");
                        if (game::CLuaManager::s_availableLuaScriptFiles.size() != 0)
                        {
                            createLuaSelectionWindow();
                            return false;
                        }
                        break;
                    }

                    Mode = MODE_START_GAME;
                    CameraTargetTarget =
                        SceneManager->getSceneNodeFromId(ID_FIRST_PLANET + SelectedPlanet)->getAbsolutePosition();
                    if (GameModeWindow)
                        GameModeWindow->setVisible(false);
                    hidePopupPlanet();
                    if (HeadingText)
                        HeadingText->setText(ox::core::CString<wchar_t>().c_str());
                }
            }
            else if (event.MouseInput.Event == ox::event::EMIE_MOUSE_MOVED)
            {
                if (allowPlanetSelection())
                {
                    ox::scene::ISceneNode* node =
                        SceneManager->getSceneCollisionManager()->getSceneNodeFromScreenCoordinatesBB(mouse);
                    if (!node)
                    {
                        PlanetHovered = false;
                        return false;
                    }

                    int planet = node->getID() - ID_FIRST_PLANET;
                    if (planet >= 0 && planet < PLANET_COUNT)
                    {
                        int previous = SelectedPlanet;
                        PlanetHovered = true;
                        SelectedPlanet = planet;
                        updatePopupPlanet(previous != planet,
                            SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(
                                node->getAbsolutePosition()));
                    }
                    else if (planet == PLANET_COUNT)
                    {
                        PlanetHovered = true;
                        SelectedPlanet = PLANET_COUNT;
                        hidePopupPlanet();
                    }
                }
                else if (Mode == MODE_GAME_MODE_SELECT && SelectedPlanet >= 0 && SelectedPlanet < PLANET_COUNT)
                {
                    for (int i = 0; i < GAME_MODE_COUNT; ++i)
                    {
                        if (i == HoveredGameMode || !GameModeButtons[i] ||
                            !GameModeButtons[i]->getAbsolutePosition().isPointInside(mouse))
                            continue;

                        HoveredGameMode = i;
                        if (GameModeTitle)
                        {
                            GameModeTitle->setText(L" ");
                            GameModeTitle->setParagraphIcon(GAME_MODE_ICONS[i],
                                Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat",
                                true), true);
                            GameModeTitle->activateProgressiveReveal(0);
                        }
                        if (GameModeDescription)
                            GameModeDescription->setText(
                                settings::gp_systemConfig->getLocalizedText(GAME_MODE_DESCRIPTIONS[i]).c_str());
                        if (GameModeStats[0])
                        {
                            ox::core::CString<wchar_t> text = settings::gp_systemConfig->getLocalizedText(MODE_STATS[0]);
                            GameModeStats[0]->setText(text.c_str());
                            GameModeStats[0]->setParagraphIcon(MODE_STRATEGY_ICONS[i],
                                Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat",
                                true), true);
                            GameModeStats[0]->activateProgressiveReveal(0);
                        }
                        if (GameModeStats[1])
                        {
                            ox::core::CString<wchar_t> text = settings::gp_systemConfig->getLocalizedText(MODE_STATS[1]);
                            GameModeStats[1]->setText(text.c_str());
                            GameModeStats[1]->setParagraphIcon(MODE_TACTICS_ICONS[i],
                                Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat",
                                true), true);
                            GameModeStats[1]->activateProgressiveReveal(0);
                        }
                        if (GameModeStats[2])
                        {
                            ox::core::CString<wchar_t> text = settings::gp_systemConfig->getLocalizedText(MODE_STATS[2]);
                            GameModeStats[2]->setText(text.c_str());
                            GameModeStats[2]->setParagraphIcon(MODE_PRESSURE_ICONS[i],
                                Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat",
                                true), true);
                            GameModeStats[2]->activateProgressiveReveal(0);
                        }
                        if (GameModeStats[3])
                        {
                            ox::core::CString<wchar_t> text = settings::gp_systemConfig->getLocalizedText(MODE_STATS[3]);
                            GameModeStats[3]->setText(text.c_str());
                            GameModeStats[3]->setParagraphIcon(MODE_DESIGN_ICONS[i],
                                Driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/harvestMenu.dat",
                                true), true);
                            GameModeStats[3]->activateProgressiveReveal(0);
                        }
                        return false;
                    }
                }
            }
        }
        break;
    case ox::event::EET_KEY_INPUT_EVENT:
        if (event.KeyInput.Event == ox::event::EKIE_KEY_PRESSED_DOWN)
        {
            Keys[event.KeyInput.Key] = true;
            if (event.KeyInput.Key == ox::KEY_KEY_U && (event.KeyInput.Control || event.KeyInput.Shift))
                NextState = EGS_SHUTTLE_RACE;
        }
        else if (event.KeyInput.Event == ox::event::EKIE_KEY_LEFT_UP)
            Keys[event.KeyInput.Key] = false;
        break;
    case ox::event::EET_DEVICE_EVENT:
        if (event.DeviceEvent.Type == ox::event::EDE_FULLSCREEN_TOGGLED)
            realignGui();
        break;
    case ox::event::EET_USER_EVENT:
        switch (event.UserEvent.UserData1)
        {
        case ECE_SHOW_WELCOME_DIALOG:
            InfoDialog->setVisible(true, gui::CMenuInfoDialog::EIM_WELCOME);
            return true;
        case ECE_START_GAME:
            NextState = EGS_PLAY;
            return true;
        case ECE_RESTART_MAIN_MENU:
            NextState = EGS_MAIN_MENU;
            return true;
        case ECE_START_SHUTTLE_RACE:
            NextState = EGS_SHUTTLE_RACE;
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

void CMainMenuState::enterPlanetSelectMode()
{
    PlanetHovered = false;
    Mode = MODE_PLANET_SELECT;
    CameraPositionTarget = PLANET_SELECT_POSITION;
    CameraTargetTarget = PLANET_SELECT_VIEW;
    if (GameModeWindow)
        GameModeWindow->setVisible(false);
    hidePopupPlanet();
    if (HeadingText)
    {
        ox::core::CString<wchar_t> text(L"Please Select Destination");
        HeadingText->setText(text.c_str());
    }
}

void CMainMenuState::enterNeutralMode()
{
    PlanetHovered = false;
    Mode = MODE_NEUTRAL;
    hidePopupPlanet();
    if (GameModeWindow)
        GameModeWindow->setVisible(false);
    if (HeadingText)
        HeadingText->setText(L"");
}

void CMainMenuState::hidePopupPlanet()
{
    if (PopupPlanet)
    {
        PopupPlanet->remove();
        PopupPlanet = 0;
    }
}

void CMainMenuState::createLockedPlanetMessageBox(int planet)
{
    int requiredScore = 46;
    if (planet == 2)
        requiredScore = 84;
    int score = settings::gp_profileManager->getCurrentProfile()->getAchievementScore();
    int rating = settings::gp_profileManager->getCurrentProfile()->getAchievementRating(score);

    ox::core::CString<wchar_t> text = L"#1";
    text += settings::gp_systemConfig->getLocalizedText(L"menu:planetLockedInfo");
    text += L"#7\n";
    text += settings::gp_systemConfig->getLocalizedText(L"menu:planetLockedTarget");

    ox::core::CString<wchar_t> target = settings::gp_systemConfig->getLocalizedText(
        settings::ACHIEVEMENT_RATING_NAMES[settings::gp_profileManager->getCurrentProfile()->getAchievementRating(
            requiredScore)]);
    target += L" (";
    target.append(requiredScore);
    target += L")";
    text += L" ";
    text += target;
    text += L"\n";
    text += settings::gp_systemConfig->getLocalizedText(L"menu:planetLockedCurrent");

    ox::core::CString<wchar_t> current =
        settings::gp_systemConfig->getLocalizedText(settings::ACHIEVEMENT_RATING_NAMES[rating]);
    current += L" (";
    current.append(score);
    current += L")";
    text += L" ";
    text += current;

    GUIEnvironment->addMessageBox(L"", text.c_str(), true, ox::gui::EMBF_OK, 0, -1);
}

void CMainMenuState::enterGameModeSelectMode(int planet)
{
    PlanetHovered = false;
    Mode = MODE_GAME_MODE_SELECT;
    SelectedPlanet = planet;
    if (HeadingText)
    {
        ox::core::CString<wchar_t> text(L"Please Select Game Mode");
        HeadingText->setText(text.c_str());
    }
    hidePopupPlanet();

    int x = (ScreenSize.Width - 700) / 2;
    int y = ButtonGroupRect.UpperLeftCorner.Y - 405;
    if (!GameModeWindow)
        GameModeWindow = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(x, y, x + 700, y + 400), RootElement);
    GameModeWindow->moveTo(ox::core::CPosition2d<int>(x, y));
    GameModeWindow->removeAllChildren();

    ox::core::CString<wchar_t> backText = settings::gp_systemConfig->getLocalizedText(L"menu:back");
    ox::gui::IGUIButton* back = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 90, 20), GameModeWindow, ID_BACK,
        backText.c_str());
    int backY = 400 - back->getRelativePosition().getHeight();
    int bottom = backY - 10;
    back->moveTo(ox::core::CPosition2d<int>(0, backY));

    ox::gui::IGUILayout* info = GUIEnvironment->addFrame(ox::core::CRect<int>(0, 0, 200, 200), GameModeWindow, -1);
    info->setAnimations(MenuPackage, "Black");
    fillLayoutWithPlanetInfo(info, SelectedPlanet, false);
    info->moveTo(ox::core::CPosition2d<int>(0, bottom - info->getRelativePosition().getHeight()));

    ox::gui::IGUIElement* modes = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 200, 200), GameModeWindow);
    ox::gui::IGUIElement* buttons = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 200, 200), modes);
    for (int i = 0; i < GAME_MODE_COUNT; ++i)
    {
        GameModeButtons[i] = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 100, 30), buttons);
        GameModeButtons[i]->setReportOnDraw(true);
        GameModeButtons[i]->setID(ID_FIRST_GAME_MODE + i);
        GameModeButtons[i]->LayoutFlags = "br";
    }
    ((ox::gui::IGUILayout*)buttons)->sortRiver(true, 10, 10, false);

    ox::gui::IGUILayout* description =
        (ox::gui::IGUILayout*)GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 200, 200), modes);
    description->setID(ID_GAME_MODE_INFO);
    description->setReportOnDraw(true);
    GameModeTitle = GUIEnvironment->addStaticText(L"", ox::core::CRect<int>(0, 0, 150, 60), false, true, description,
        -1, L"");
    GameModeTitle->LayoutFlags = "center";
    GameModeDescription = GUIEnvironment->addStaticText(L"", ox::core::CRect<int>(0, 0, 200, 150), false, true,
        description, -1, L"");
    GameModeDescription->LayoutFlags = "br left";
    GameModeDescription->setOverrideFont(SmallFont);
    GameModeStats[0] = GUIEnvironment->addStaticText(L"", 200, description, SmallFont, -1, L"");
    GameModeStats[0]->LayoutFlags = "br";
    GameModeStats[1] = GUIEnvironment->addStaticText(L"", 200, description, SmallFont, -1, L"");
    GameModeStats[1]->LayoutFlags = "br";
    GameModeStats[2] = GUIEnvironment->addStaticText(L"", 200, description, SmallFont, -1, L"");
    GameModeStats[2]->LayoutFlags = "br";
    GameModeStats[3] = GUIEnvironment->addStaticText(L"", 200, description, SmallFont, -1, L"");
    GameModeStats[3]->LayoutFlags = "br";
    description->sortRiver(true, 5, 5, false);

    ((ox::gui::IGUILayout*)modes)->sortRiver(true, 0, 0, false);
    ox::core::CRect<int> modesRect = modes->getRelativePosition();
    modes->moveTo(ox::core::CPosition2d<int>(700 - modesRect.getWidth(), bottom - modesRect.getHeight()));
    buttons->moveTo(ox::core::CPosition2d<int>(0, (modesRect.getHeight() - buttons->getRelativePosition().getHeight()) / 2));
    GameModeWindow->setVisible(true);
}

void CMainMenuState::createLuaSelectionWindow()
{
    if (ModalWindow)
        ModalWindow->remove();

    ModalWindow = GUIEnvironment->addWindow(ox::core::CRect<int>(0, 0, 200, 300), true, L"", 0, -1);
    ox::gui::IGUIStaticText* title = GUIEnvironment->addStaticText(
        settings::gp_systemConfig->getLocalizedText(L"menu:selectMods").c_str(), 320, ModalWindow,
        GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/largeFont.fnt"), -1, L"");
    title->LayoutFlags = "center br";
    title->setTextAlignment(ox::gui::EFHA_CENTER, ox::gui::EFVA_TOP);

    ox::gui::IGUIListBox* list =
        GUIEnvironment->addListBox(ox::core::CRect<int>(0, 0, 300, 400), ModalWindow, -1, false);
    list->EventReceiver = this;
    for (unsigned int i = 0; i < game::CLuaManager::s_availableLuaScriptFiles.size(); ++i)
    {
        ox::gui::IGUIElement* row =
            GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 250, 40), list->getListParent());
        row->setID(ID_FIRST_MOD + i);
        ox::gui::IGUILayout* checkGroup =
            (ox::gui::IGUILayout*)GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(0, 0, 20, 40), row);
        ox::gui::IGUIElement* iconGroup = GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(20, 0, 60, 40), row);
        ox::gui::IGUILayout* textGroup =
            (ox::gui::IGUILayout*)GUIEnvironment->addLayoutGroup(ox::core::CRect<int>(60, 0, 250, 40), row);

        game::SLuaMod& mod = game::CLuaManager::s_availableLuaScriptFiles[i];
        GUIEnvironment->addCheckBox(mod.Enabled, ox::core::CRect<int>(1, 11, 19, 29), checkGroup, ID_FIRST_MOD + i,
            0)->LayoutFlags = "br";
        checkGroup->sortRiver(false, 0, 11, false);

        if (mod.HasIcon)
        {
            ox::gui::IGUIImage* icon =
                GUIEnvironment->addImage(ox::core::CRect<int>(4, 4, 36, 36), iconGroup, -1, L"");
            ox::core::CString<char> iconPath = mod.Path;
            iconPath += "favicon.tga";
            icon->setImage(Driver->getTexture(iconPath.c_str()));
        }

        GUIEnvironment->addStaticText(mod.Name.c_str(), "center", textGroup,
            GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/boldFont.fnt"), -1);
        GUIEnvironment->addStaticText(mod.Author.c_str(), "br", textGroup,
            GUIEnvironment->getFont("$GAME_RESOURCES$/harvestClientData/gfx/smallFont.fnt"), -1);
        textGroup->sortRiver(false, 0, 0, false);
    }
    list->LayoutFlags = "br";
    list->sortItems(true);

    GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 400, 400), ModalWindow, ID_MODS_CANCEL, L"Cancel")->LayoutFlags =
        "br";
    GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 400, 400), ModalWindow, ID_MODS_OK, L"Ok");
    ModalWindow->sortRiver(true, 5, 5, false);
    ModalWindow->centerOnParent();
    ModList = list;
}

void CMainMenuState::renderFirst()
{
    if (LoadingScreen)
        LoadingScreen->render(Device, Driver);
}

void CMainMenuState::render()
{
    Driver->beginScene(true, true, ox::video::SColor(0xff000000));
    SceneManager->drawAll();

    if (Mode == MODE_NEUTRAL && Sprites[SPRITE_LOGO])
    {
        ox::core::CPosition2d<int> position(ScreenSize.Width / 2, (ScreenSize.Height - 600) / 2 + 30);
        Sprites[SPRITE_LOGO]->draw(position, 0, ox::video::SColor(0xffffffff));
    }

    GUIEnvironment->drawAll();

    if (FadeAlpha > 0.0f)
        Driver->draw2DRectangle(ox::video::SColor((int)FadeAlpha << 24),
            ox::core::CRect<int>(0, 0, ScreenSize.Width, ScreenSize.Height), 0);

    Driver->endScene();
}

void CMainMenuState::createDemoMessageBox()
{
    if (ModalWindow)
        ModalWindow->remove();

    ModalWindow = GUIEnvironment->addWindow(ox::core::CRect<int>(0, 0, 200, 200), true, L"", RootElement,
        ID_DEMO_WINDOW);
    ox::gui::IGUIImage* logo = GUIEnvironment->addImage(ox::core::CRect<int>(0, 0, 500, 400), ModalWindow, -1, 0);
    logo->LayoutFlags = "br center";
    logo->setAnimation("HarvestLogo", MenuPackage);

    ox::gui::IGUIStaticText* text = GUIEnvironment->addStaticText(
        settings::gp_systemConfig->getLocalizedText(L"menu:gameModeNotAvailableDemo").c_str(), 400, ModalWindow, 0, -1,
        L"");
    text->LayoutFlags = "br";

    ox::gui::IGUIButton* buy = GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 400, 400), ModalWindow,
        ID_DEMO_BUY, settings::gp_systemConfig->getLocalizedText(L"menu:demoBuy").c_str());
    buy->LayoutFlags = "br";
    GUIEnvironment->addButton(ox::core::CRect<int>(0, 0, 400, 400), ModalWindow, ID_DEMO_LATER,
        settings::gp_systemConfig->getLocalizedText(L"menu:demoLater").c_str());

    ModalWindow->sortRiver(true, 10, 10, false);
    ModalWindow->centerOnParent();
}

} // end namespace states
} // end namespace harvest
