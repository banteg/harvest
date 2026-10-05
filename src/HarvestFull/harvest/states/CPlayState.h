// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Method names follow the Mac symbols; member names are ours.

#ifndef HARVEST_STATES_CPLAYSTATE_H
#define HARVEST_STATES_CPLAYSTATE_H

#include <list>
#include "ox/TArray.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CRect.h"
#include "ox/core/CString.h"
#include "ox/game/CGameState.h"
#include "ox/video/IParticleEngineCallback.h"
#include "ox/video/SColor.h"
#include "harvest/entity/CBuildableItems.h"
#include "harvest/entity/CEntityManager.h"

namespace ox {
class IUnknown;
namespace entity {
class COxEntity;
struct SEntityReference;
} // end namespace entity
namespace gui {
class IGUIButton;
class IGUICheckBox;
class IGUIElement;
class IGUIFont;
class IGUILayout;
class IGUIStaticText;
} // end namespace gui
namespace video {
class ISpriteAnimationState;
class ISpritePackage;
class ITexture;
} // end namespace video
} // end namespace ox

namespace harvest {
namespace entity { class CEntity; }
namespace game {
class CLuaManager;
class CScenario;
class CThreatLevel;
struct SInfoLineMessage;
} // end namespace game
namespace settings { class CHarvestProfile; }
namespace gui {
class CAchievementsScreen;
class CGuiInfoLines;
class CIngameMenuScreen;
class CPriorityScreen;
class CSaveGameScreen;
class CSettingsScreen;
class CStoryScreen;
} // end namespace gui

namespace states {

class CLoadingScreen;

//! Whether the threat level is shown in a game mode.
bool displayThreatLevelForGameMode(int gameMode);
//! Whether the game timer is shown in a game mode.
bool displayTimerForGameMode(int gameMode);

//! A blip on the minimap, at a world position; it shrinks to a small square while Time runs out.
struct SMinimapMarker
{
    float X;
    float Y;
    float Time;
};

//! The game state of a running game.
class CPlayState : public ox::game::CGameState, public ox::video::IParticleEngineCallback
{
public:
    CPlayState();
    virtual ~CPlayState();

    virtual bool OnEvent(const ox::event::SEvent& event);
    virtual int firstInit(ox::IOxDevice* device);
    virtual void renderFirst();
    virtual int secondInit();
    virtual int updateState(float time);
    virtual void render();

    virtual void addParticleEntity(ox::video::IParticleState* state, const ox::core::CVector3d<float>& position);
    virtual int getOnDieMarkerAtPos(const ox::core::CVector3d<float>& position);
    virtual void playParticleSound(const char* sound, const ox::core::CVector3d<float>& position);

    ox::gui::IGUILayout* getPopupForGuiButton(const wchar_t* text);
    ox::gui::IGUILayout* getPopupForBuildButton(const wchar_t* name, const wchar_t* description, int energy,
        int minerals);
    bool readStateFromFile(const char* filename);
    bool initializeNewGame();
    void realignGui();
    void playPlanetMusic();
    void displayTimeVictoryMessage();
    void displayRecordMessage(int value, bool minerals);
    void displayInsaneRewardMessage();
    void clearSelectedEntity();
    void newSelectedEntity();
    void updateMultiSelectionReferences();
    void checkWaveReward();
    void updatePlacementPosition();
    void buyBuildingAtPlacementPos();
    void updateRecycleBuilding();
    void setBuildAction(int index);
    void setGameSpeed(int speed);
    void setNoneAction(bool clearSelection);
    void replaceSelectedMissileTurret(int entityType);
    void makeSelectedDeathstarTower();
    void unmakeSelectedDeathstarTower();
    void sellAllHarvesters();
    void replaceSelectedProducer(int entityType);
    void toggleWaveList();
    void toggleCreativeList();
    void launchWaveLevel(int wave);
    void setPlaceAlienAction(int alienType);
    void replaceBuildingWithType(entity::CEntity* building, const char* entityId);
    void togglePause();
    void renderMinimap();
    void renderWaveButton(ox::gui::IGUIElement* button, int numAliens, int firstAlien, bool* aliens,
        const ox::core::CString<wchar_t>& text);
    void setRecycleAction();
    ox::core::CPosition2d<float> getWorldPos(ox::core::CPosition2d<int> position);
    ox::core::CDimension2d<int> getViewSize();
    void performRectangleSelection(ox::core::CPosition2d<float> corner1, ox::core::CPosition2d<float> corner2,
        bool add, int entityType);
    void buildingShiftSelected(entity::CEntity* building);
    void sellEntity(entity::CEntity* building);
    void placeCurrentAlienSelection(const ox::core::CPosition2d<float>& position);
    void setWaveListToggle(bool visible);
    void setCreativeListToggle(bool visible);
    bool isMultiSelected();
    void changeConstructionSelection(bool next);
    void addInfoLine(game::SInfoLineMessage* message, bool sound);
    bool writeStateToFile(const char* filename, const wchar_t* description);
    void addInfoLine(const ox::core::CString<wchar_t>& text);
    void toggleMenuPos();
    void renderRangeCircleForEntity(ox::entity::COxEntity* entity, const ox::core::CRect<int>& viewPort);
    void renderRangeCircle(const ox::core::CPosition2d<float>& position, float range, ox::video::SColor color,
        const ox::core::CRect<int>& viewPort);
    void eraseGameObjects();
    void displayWelcomeMessage();
    void addToSelection(entity::CEntity* building);
    void loadWaveListSprites();

    //! Which keys are held down, by key code.
    static bool m_keys[256];

private:
    //! The sprites of the in-game gui, indexed by EGUI_SPRITE.
    enum EGUI_SPRITE
    {
        GS_BOTTOM_LEFT_BACKGROUND,
        GS_BOTTOM_RIGHT_BACKGROUND,
        GS_BOTTOM_CENTER_BACKGROUND,
        GS_TOP_RIGHT_BACKGROUND,
        GS_THREAT_LEVEL_BACKGROUND,
        GS_TIME_BACKGROUND,
        GS_OBJECTIVES_BACKGROUND,
        GS_TOP_LEFT_BACKGROUND,
        GS_PROGRESS_BAR,
        GS_MINIMAP_TOP_LEFT,
        GS_MINIMAP_TOP,
        GS_MINIMAP_TOP_RIGHT,
        GS_MINIMAP_LEFT,
        GS_MINIMAP_RIGHT,
        GS_MINIMAP_BOTTOM_LEFT,
        GS_MINIMAP_BOTTOM,
        GS_MINIMAP_BOTTOM_RIGHT,
        GS_MINIMAP_BACKGROUND,
        GS_MINERALS_BACKGROUND,
        GS_DAMAGE_BAR_BACKGROUND,
        GS_DAMAGE_BAR,
        GS_ICON_ENERGY,
        GS_ICON_CREDITS,
        GS_ICON_OBJECTIVE,
        GS_COUNT
    };

    enum ELIST
    {
        LIST_WAVES,
        LIST_CREATIVE,
        LIST_COUNT
    };

    enum
    {
        WAVE_COUNT = 14,
        GUI_ELEMENT_COUNT = 58
    };

    //! Ids of the in-game gui elements; GuiElements holds the ones below GUI_ELEMENT_COUNT by id.
    enum EGUI_ID
    {
        GUI_ID_PRIORITIES = 5,
        GUI_ID_SPEED_PAUSE,
        GUI_ID_SPEED_SLOW,
        GUI_ID_SPEED_NORMAL,
        GUI_ID_SPEED_DOUBLE,
        GUI_ID_SPEED_FOUR,
        GUI_ID_BUILDINGS_LEFT,
        GUI_ID_BUILDINGS_RIGHT,
        GUI_ID_MENU,
        GUI_ID_DESELECT = 15,
        GUI_ID_UNLINK,
        GUI_ID_OVERCHARGE,
        GUI_ID_EAGLE,
        GUI_ID_TEMPEST,
        GUI_ID_DEATHSTAR,
        GUI_ID_UNLINK_DEATHSTAR,
        GUI_ID_END_LASER,
        GUI_ID_SPEED_BUILD,
        GUI_ID_UNLINK_SPEED_BUILD,
        GUI_ID_REPLACE_PRODUCER,
        GUI_ID_SELL_HARVESTERS,
        //! The action buttons that scripts add to buildings.
        GUI_ID_FIRST_LUA_ACTION,
        GUI_ID_WAVE_SEND = 36,
        GUI_ID_CREATIVE_PLACE,
        GUI_ID_FIRST_WAVE,
        GUI_ID_FIRST_CREATIVE_ALIEN = 48
    };

    //! Ids of the gui elements that report their drawing.
    enum
    {
        GUI_ID_ENERGY_POPUP = 1239,
        GUI_ID_MINERALS_POPUP = 1240
    };

    int m_050;
    bool m_054;
    ox::core::CDimension2d<int> ScreenSize;
    ox::core::CDimension2d<float> ScreenSizeF;
    CLoadingScreen* LoadingScreen;
    unsigned int InitStep;
    ox::gui::IGUIFont* BoldFont;
    ox::gui::IGUIFont* SmallFont;
    ox::gui::IGUIFont* NumberFont;
    int BoldFontHeight;
    int SmallFontHeight;
    ox::video::ISpritePackage* IngamePackage;
    ox::video::ISpritePackage* MenuPackage;
    ox::core::CPosition2d<float> ViewPosition;
    //! The part of the world on screen.
    ox::core::CRect<float> VisibleArea;
    int m_0c0;
    int m_0c4;
    int Action;
    entity::CEntity* SelectedEntity;
    //! The building under the cursor while recycling.
    entity::CEntity* RecycleTarget;
    int m_0e0;
    int SelectedEntityType;
    int RecycleTargetId;
    ox::entity::COxEntity* m_0f0;
    int m_0f8;
    int m_0fc;
    int m_100;
    bool m_104;
    //! Where the last building was placed; the next one is kept within a link's reach of it.
    ox::core::CPosition2d<float> LastPlacement;
    bool HasLastPlacement;
    bool m_111;
    int AlienSelection;
    ox::TArray<ox::entity::SEntityReference*> MultiSelection;
    bool m_130;
    bool m_131;
    bool PlacementOk;
    bool m_133;
    ox::core::CPosition2d<float> PlacementPosition;
    int BuildSelection;
    entity::CBuildableItems BuildableItems;
    ox::video::ISpriteAnimationState* RangeCircle;
    ox::video::ISpriteAnimationState* Selector;
    ox::video::ISpriteAnimationState* RecycleSelector;
    entity::SEnergyBeam Beam180;
    entity::SEnergyBeam Beam1c8;
    entity::SEnergyBeam Beam210;
    entity::SEnergyBeam Beam258;
    bool m_2a0;
    game::CThreatLevel* ThreatLevel;
    game::CScenario* Scenario;
    int GameMode;
    int GameSpeed;
    int m_2c0;
    bool m_2c4;
    bool m_2c5;
    int m_2c8;
    //! The credits when the game ended.
    int Minerals;
    bool m_2d0;
    ox::core::CRect<int> MinimapRect;
    ox::video::ISpriteAnimationState* MinimapDot;
    //! The minimap is drawn into this texture every two seconds when UseMinimapTexture is set.
    ox::video::ITexture* MinimapTexture;
    bool UseMinimapTexture;
    float MinimapUpdateTime;
    std::list<SMinimapMarker> MinimapMarkers;
    int m_310;
    int m_314;
    int m_318;
    float GameTime;
    bool m_320;
    bool m_321;
    bool m_322;
    int m_324;
    bool m_328;
    //! Counts down while the not-enough-credits warning flashes.
    float DenialTime;
    float m_330;
    void* m_338;
    ox::gui::IGUIStaticText* m_340;
    ox::gui::IGUIStaticText* m_348;
    ox::gui::IGUIStaticText* m_350;
    ox::video::ISpriteAnimationState* GuiSprites[GS_COUNT];
    //! The alien icons of the wave buttons, by alien type.
    ox::video::ISpriteAnimationState* WaveIcons[WAVE_COUNT];
    ox::video::ISpriteAnimationState* m_488[WAVE_COUNT];
    //! The bar along the bottom edge and the panel in the top right corner.
    ox::gui::IGUIElement* BottomBar;
    //! The action buttons of the selected building.
    ox::gui::IGUIElement* ActionPanel;
    ox::gui::IGUIElement* TopBar;
    int TimerWidth;
    int MinimapWidth;
    int MinimapHeight;
    //! Whether the screen is large enough for the minimap.
    bool ShowMinimap;
    //! The parts of the bottom bar drawn by the bottom sprites.
    ox::core::CRect<int> BarLeftArea;
    ox::core::CRect<int> BarRightArea;
    ox::core::CRect<int> BarCenterArea;
    ox::gui::IGUIElement* GuiElements[GUI_ELEMENT_COUNT];
    ox::gui::IGUICheckBox* RecycleButton;
    ox::gui::IGUIElement* m_728;
    ox::gui::IGUIStaticText* SelectedNameText;
    ox::gui::IGUIElement* m_738;
    ox::gui::IGUIElement* m_740;
    //! The build buttons, which scroll inside BuildingsArea.
    ox::gui::IGUILayout* BuildingsArea;
    ox::gui::IGUILayout* BuildingsList;
    bool m_758;
    int m_75c;
    int m_760;
    bool m_764;
    int m_768;
    int m_76c;
    int m_770;
    bool m_774;
    //! The wave and creative alien lists at the right edge, indexed by ELIST.
    ox::gui::IGUILayout* ListGroups[LIST_COUNT];
    ox::gui::IGUILayout* ListContents[LIST_COUNT];
    ox::gui::IGUIButton* ListButtons[LIST_COUNT];
    bool ListVisible[LIST_COUNT];
    ox::gui::IGUIElement* m_7b0;
    gui::CSettingsScreen* SettingsScreen;
    gui::CPriorityScreen* PriorityScreen;
    gui::CIngameMenuScreen* IngameMenuScreen;
    gui::CSaveGameScreen* SaveGameScreen;
    gui::CStoryScreen* StoryScreen;
    gui::CAchievementsScreen* AchievementsScreen;
    gui::CGuiInfoLines* InfoLines;
    //! When the game was started, as returned by time(), and a checksum of it for the highscores.
    int StartTime;
    int RandomValue;
    settings::CHarvestProfile* Profile;
    ox::core::CString<wchar_t> PlayerName;
    ox::core::CString<wchar_t> PlayerGroup;
    int ParticleSetting;
    float ScrollSpeed;
    bool m_828;
    bool m_829;
    int m_82c;
    int m_830;
    void* m_838;
    game::CLuaManager* LuaManager;
};

} // end namespace states
} // end namespace harvest

#endif
