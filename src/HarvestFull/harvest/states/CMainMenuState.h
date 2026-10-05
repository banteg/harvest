// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_STATES_CMAINMENUSTATE_H
#define HARVEST_STATES_CMAINMENUSTATE_H

#include "ox/core/CDimension2d.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CRect.h"
#include "ox/core/CVector3d.h"
#include "ox/game/CGameState.h"

namespace ox {
namespace gui {
class IGUIElement;
class IGUIFont;
class IGUILayout;
class IGUIListBox;
class IGUIStaticText;
class IGUIButton;
} // end namespace gui
namespace scene {
class IAnimatedMeshSceneNode;
class IBillboardSceneNode;
class ICameraSceneNode;
class ISceneNode;
} // end namespace scene
namespace video {
class ISpriteAnimationState;
class ISpritePackage;
} // end namespace video
} // end namespace ox

namespace harvest {
namespace gfx { class CScatterShader; }
namespace gui {
class CAchievementsScreen;
class CHighscoreScreen;
class CMenuInfoDialog;
class CProfileScreen;
class CSaveGameScreen;
class CSettingsScreen;
class CStatisticsScreen;
} // end namespace gui
namespace states {

class CLoadingScreen;

//! The main menu: the menu console over the planet scene, the planet and game mode selection.
class CMainMenuState : public ox::game::CGameState
{
public:
    CMainMenuState();
    virtual ~CMainMenuState();

    virtual bool OnEvent(const ox::event::SEvent& event);
    virtual int firstInit(ox::IOxDevice* device);
    virtual void renderFirst();
    //! Loads one step per call; returns 2 while loading, 1 on failure and 0 when done.
    virtual int secondInit();
    //! Returns the state to switch to, or 0 to stay.
    virtual int updateState(float time);
    virtual void render();

private:
    //! The menu modes.
    enum
    {
        MODE_NEUTRAL,
        MODE_PLANET_SELECT,
        MODE_GAME_MODE_SELECT,
        //! The camera flies to the chosen planet before the game starts.
        MODE_START_GAME
    };

    //! GUI element and scene node ids.
    enum
    {
        ID_NEW_GAME = 1445,
        ID_LOAD_GAME,
        ID_SETTINGS,
        ID_PROFILE,
        ID_HIGHSCORES,
        ID_AWARDS,
        ID_EXIT,
        ID_BACK = 1457,
        //! The scene nodes of the three planets.
        ID_FIRST_PLANET,
        ID_ATRUM = ID_FIRST_PLANET + 3,
        ID_SHUTTLE_RACE_BOX,
        //! The five game mode buttons.
        ID_FIRST_GAME_MODE,
        ID_BUTTON_GROUP = ID_FIRST_GAME_MODE + 5,
        //! Elements drawn over a dark background; no 1.18 element has the second id.
        ID_GAME_MODE_INFO,
        ID_SHADED_PANEL,
        ID_DEMO_WINDOW,
        ID_DEMO_BUY,
        ID_DEMO_LATER,
        ID_MODS_OK,
        ID_MODS_CANCEL,
        //! The check boxes of the mods.
        ID_FIRST_MOD
    };

    enum
    {
        SPRITE_CONSOLE,
        SPRITE_CONSOLE_TEXT,
        SPRITE_CONSOLE_TILE,
        SPRITE_MODE_NORMAL,
        SPRITE_MODE_WAVE,
        SPRITE_MODE_INSANE,
        SPRITE_MODE_RUSH,
        SPRITE_MODE_CREATIVE,
        SPRITE_MODE_SELECTOR,
        SPRITE_LOGO,
        SPRITE_COUNT
    };

    //! The console buttons, from left to right.
    enum
    {
        BUTTON_PROFILE,
        BUTTON_SETTINGS,
        BUTTON_HIGHSCORES,
        BUTTON_NEW_GAME,
        BUTTON_LOAD_GAME,
        BUTTON_AWARDS,
        BUTTON_EXIT,
        BUTTON_COUNT
    };

    static const int PLANET_COUNT = 3;
    static const int GAME_MODE_COUNT = 5;

    void realignGui();
    void createEditProfileWindow(bool firstProfile);
    void createProfileListWindow();
    bool allowPlanetSelection();
    //! Moves the planet info popup above position, creating it first when needed or asked to.
    void updatePopupPlanet(bool recreate, const ox::core::CPosition2d<int>& position);
    void enterPlanetSelectMode();
    void enterNeutralMode();
    void hidePopupPlanet();
    void createLockedPlanetMessageBox(int planet);
    void enterGameModeSelectMode(int planet);
    void createLuaSelectionWindow();
    void fillLayoutWithPlanetInfo(ox::gui::IGUILayout* layout, int planet, bool popup);
    void createDemoMessageBox();

    int NextState;
    bool Keys[256];
    CLoadingScreen* LoadingScreen;
    int InitStep;
    ox::gui::IGUIFont* BoldFont;
    ox::gui::IGUIFont* SmallFont;
    ox::core::CDimension2d<int> ScreenSize;
    ox::video::ISpritePackage* MenuPackage;
    ox::gui::IGUIElement* RootElement;
    //! Neither set nor read by the 1.18 code.
    ox::gui::IGUIElement* UnusedElement;
    ox::gui::IGUIElement* ButtonGroup;
    //! The mod selection or demo window.
    ox::gui::IGUILayout* ModalWindow;
    ox::gui::IGUIListBox* ModList;
    ox::gui::IGUIStaticText* HeadingText;
    gui::CSettingsScreen* SettingsScreen;
    gui::CHighscoreScreen* HighscoreScreen;
    gui::CStatisticsScreen* StatisticsScreen;
    gui::CSaveGameScreen* SaveGameScreen;
    gui::CMenuInfoDialog* InfoDialog;
    gui::CProfileScreen* ProfileScreen;
    gui::CAchievementsScreen* AchievementsScreen;
    //! Neither set nor read by the 1.18 code.
    int UnusedValue;
    //! Each planet and its atmosphere.
    ox::scene::IAnimatedMeshSceneNode* PlanetNodes[PLANET_COUNT * 2];
    ox::scene::ICameraSceneNode* Camera;
    //! The planet under the mouse or chosen, 3 for Atrum.
    int SelectedPlanet;
    int HoveredGameMode;
    int Mode;
    bool PlanetHovered;
    ox::core::CVector3d<float> CameraPosition;
    ox::core::CVector3d<float> CameraPositionTarget;
    ox::core::CVector3d<float> CameraSpeed;
    ox::core::CVector3d<float> CameraTarget;
    ox::core::CVector3d<float> CameraTargetTarget;
    ox::core::CVector3d<float> CameraTargetSpeed;
    float FieldOfView;
    float Time;
    //! The zoom and fade time of the game start.
    float ZoomTime;
    float FadeTime;
    float FadeAlpha;
    gfx::CScatterShader* Shaders[PLANET_COUNT * 2];
    float PlanetDistances[PLANET_COUNT];
    ox::scene::ISceneNode* SkyBox;
    ox::scene::IBillboardSceneNode* Sun;
    ox::video::ISpriteAnimationState* Sprites[SPRITE_COUNT];
    ox::gui::IGUIButton* Buttons[BUTTON_COUNT];
    ox::core::CRect<int> ButtonGroupRect;
    ox::gui::IGUILayout* PopupPlanet;
    ox::gui::IGUIElement* GameModeWindow;
    ox::gui::IGUIElement* GameModeButtons[GAME_MODE_COUNT];
    ox::gui::IGUIStaticText* GameModeTitle;
    ox::gui::IGUIStaticText* GameModeDescription;
    //! The strategy, tactics, pressure and design ratings.
    ox::gui::IGUIStaticText* GameModeStats[4];
};

} // end namespace states
} // end namespace harvest

#endif
