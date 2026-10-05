// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Method and global names are from the Mac symbols; member names are ours.

#ifndef HARVEST_GAME_CLUAMANAGER_H
#define HARVEST_GAME_CLUAMANAGER_H

#include "ox/TArray.h"
#include "ox/core/CVector3d.h"
#include "ox/core/CString.h"
#include "ox/core/CRect.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CDimension2d.h"
#include "ox/video/SColor.h"
#include "lua.hpp"

namespace ox {
class IOxDevice;
namespace event { class IEventReceiver; }
namespace entity { struct SEntityReference; }
namespace io { class IFileSystem; class IFileList; class IReadFile; class IWriteFile; }
namespace video { class IVideoDriver; class ISpriteAnimationState; }
namespace gui { class IGUIFont; }
} // end namespace ox

namespace harvest {
namespace entity {
class CEntity;
class CBuildingEntity;
class CAlienEntity;
class CCreativeEntity;
} // end namespace entity

namespace game {

//! A mod: a folder with a main.lua, described by an .hmd file.
struct SLuaMod
{
    ox::core::CString<wchar_t> Name;
    ox::core::CString<wchar_t> Author;
    //! The folder name, which saves refer to the mod by.
    ox::core::CString<char> Folder;
    //! The folder path, ending in a slash.
    ox::core::CString<char> Path;
    bool Enabled;
    //! Whether the folder has a favicon.tga.
    bool HasIcon;
};

//! A hook function of creative buildings and whether it exists (0 unknown, 1 yes, 2 no).
struct SCreativeHook
{
    ox::core::CString<char> Name;
    int State;
};

//! A button that scripts add to the action bar of a building type.
struct SLuaEntityActionButton
{
    int Id;
    ox::core::CString<char> EntityId;
    ox::core::CString<char> Command;
    //! Upgrade buttons are drawn as upgrades.
    bool Upgrade;
};

//! Text, a line or a rectangle that a script draws for one frame.
struct SLuaGuiObject
{
    //! 0 text, 1 line, 2 rectangle.
    int Type;
    ox::core::CString<wchar_t> Text;
    int X;
    int Y;
    //! The line's end, the rectangle's size, or the text's alignment in Extent1.
    int Extent1;
    int Extent2;
    ox::video::SColor Color;
};

//! Runs the Lua scripts of mods and the creative mode.
class CLuaManager
{
public:
    CLuaManager(ox::IOxDevice* device, ox::event::IEventReceiver* receiver);
    ~CLuaManager();

    void setCreativeListVisible(bool visible);
    void setRushListVisible(bool visible);
    void setRushProgress(float progress);
    void setWaveListVisible(bool visible);
    void setTimerVisible(bool visible);
    void setTimerValue(float value);
    void setWaveButtonVisible(int button, bool visible);
    void setWaveButtonAliens(int button, bool* aliens);
    void setWaveButtonNumber(int button, int number);
    void setMinimumWorldBorders(const ox::core::CRect<float>& borders);
    ox::io::IFileSystem* getFileSystem();
    //! Loads a sprite animation of the sprite package file (or the game's) and returns its index.
    int createSpriteState(const char* name, const ox::core::CString<char>& packageFile);
    //! Removes a sprite state before the next frame.
    void markSpriteState(int index);
    ox::video::ISpriteAnimationState* getSpriteState(int index);
    const ox::core::CPosition2d<float>& getViewPosition() const;
    void setViewPosition(const ox::core::CPosition2d<float>& position);
    ox::core::CPosition2d<int> getMousePosition();
    void addEntityActionButton(const char* entityId, const char* command, bool upgrade);
    entity::CEntity* getSelectedBuilding();
    ox::TArray<ox::entity::SEntityReference*>* getSelectedBuildings();

    void initCommonLuaStuff();
    void includeMod(const SLuaMod& mod);
    void initLuaByScriptList();
    bool initLuaBySaveFile(ox::io::IReadFile* file, int version);
    static void refreshAvailableLuaMods(ox::IOxDevice* device);
    bool writeLuaStates(ox::io::IWriteFile* file);
    //! Pushes the hook dispatcher and the hook name; false when the scripts lack the hook.
    bool pushHooker(const char* name, int hook);
    //! Learns from the hook's result whether the scripts have it, and pops the result.
    void checkHooker(int hook);
    ox::TArray<ox::core::CString<char> >& getCompilerErrors();
    int getNumAvailableLuaMods();
    ox::TArray<SLuaMod>& getAvailableLuaMods();

    void hookNewGame();
    void runFrameFunctions(float frameDelta);
    void updateAllSpriteStates(float frameDelta);
    static void postStringAsInfo(const ox::core::CString<char>& text);
    void hookMouseClick(const ox::core::CPosition2d<float>& position, int button, bool pressed);
    void hookAlienDeath(int alienType, const ox::core::CVector3d<float>& position);
    void hookAlienSpawned(entity::CAlienEntity* alien);
    void hookBuildingPlaced(entity::CBuildingEntity* building, const char* buildingType);
    //! Tells the scripts that a construction site has become a building.
    void hookBuildingConstructed(entity::CBuildingEntity* building);
    void hookBuildingDestroyed(const char* buildingType, float x, float y, entity::CAlienEntity* alien);
    void hookBuildingSold(const char* buildingType, float x, float y, int credits);
    void hookUnitSelected(int id);
    void hookCreditsMined(entity::CBuildingEntity* miner, int mineralsId);
    void hookMinerOutOfMinerals(entity::CBuildingEntity* miner);
    //! Tells the scripts that a building sent out a spark.
    void hookEnergySparkCreated(int sparkId, entity::CBuildingEntity* building);
    void hookMissileLaunched(int missileType, entity::CBuildingEntity* turret, int targetId, float x, float y);
    //! A spark died at an overheated energy link.
    void hookEnergyLinkOverheated(entity::CBuildingEntity* link);
    void hookEnergyLinkCharging(entity::CBuildingEntity* link);
    //! An energy link finished charging and exploded at x, y.
    void hookEnergyLinkCharged(float x, float y);
    void hookMapExpanded(float left, float top, float right, float bottom);
    void hookWaveButton(int button);
    void hookTextInput(const wchar_t* text);
    void hookCreativeInit(const ox::core::CString<char>& id, entity::CCreativeEntity* building);
    bool pushCreativeHooker(const char* name, int hookSet);
    int checkCreativeHooker(const char* name, int hookSet);
    void hookCreativeUpdate(const ox::core::CString<char>& id, entity::CCreativeEntity* building, float frameDelta);
    bool isRunningMods();
    float getThreatLevelProgress();
    int getThreatLevelValue();
    void setThreatLevelProgress(float progress);
    void setThreatLevelValue(int value);
    void updateSelectedBuilding(entity::CEntity* building);
    void updateSelectedBuildings(ox::TArray<ox::entity::SEntityReference*>* buildings);
    ox::core::CPosition2d<int> worldToScreen(const ox::core::CPosition2d<float>& position);
    ox::core::CPosition2d<float> screenToWorld(const ox::core::CPosition2d<int>& position);
    void addTextObject(const ox::core::CString<wchar_t>& text, const ox::core::CPosition2d<int>& position,
        int alignment, ox::video::SColor color);
    void addLineObject(const ox::core::CPosition2d<int>& start, const ox::core::CPosition2d<int>& end,
        ox::video::SColor color);
    void addRectangleObject(const ox::core::CPosition2d<int>& position,
        const ox::core::CDimension2d<int>& size, ox::video::SColor color);
    //! Draws and then forgets the objects the scripts drew this frame.
    void renderGuiObjects(ox::video::IVideoDriver* driver, ox::gui::IGUIFont* font);
    bool isWaveButtonVisible(int button);
    bool isAlienOnButton(int button, int alien);
    int getWaveButtonNumber(int button);
    bool isCreativeListVisible();
    bool isRushListVisible();
    bool isWaveListVisible();
    bool isTimerVisible();
    float getRushProgress();
    float getTimerValue();
    const ox::core::CRect<float>& getMinimumWorldBorders();
    void removeSpriteState(int index);
    SLuaEntityActionButton* getEntityActionButton(int id, const char* entityId);
    SLuaEntityActionButton* getNextEntityActionButton(const char* entityId, SLuaEntityActionButton* previous);
    static void addModsFromFiles(ox::io::IFileList* files, ox::IOxDevice* device,
        const ox::core::CString<char>& directory);

    //! The mods found by refreshAvailableLuaMods.
    static ox::TArray<SLuaMod> s_availableLuaScriptFiles;

    // The Lua bindings of CLuaManager.cpp use the members directly.
    ox::IOxDevice* Device;
    ox::event::IEventReceiver* EventReceiver;
    //! The mods that loaded.
    ox::TArray<SLuaMod> RunningMods;
    ox::TArray<ox::core::CString<char> > CompilerErrors;
    lua_State* L;
    //! A registry reference to hook.call.
    int HookFunction;
    int HookStates[22];
    ox::TArray<SCreativeHook> CreativeHooks[2];
    float ThreatLevelProgress;
    int ThreatLevelValue;
    ox::core::CPosition2d<float> ViewPosition;
    bool CreativeListVisible;
    bool WaveListVisible;
    bool RushListVisible;
    bool TimerVisible;
    float RushProgress;
    float TimerValue;
    bool WaveButtonVisible[10];
    int WaveButtonNumber[10];
    bool WaveButtonAliens[10][14];
    entity::CEntity* SelectedBuilding;
    ox::TArray<ox::entity::SEntityReference*>* SelectedBuildings;
    ox::core::CRect<float> MinimumWorldBorders;
    ox::TArray<SLuaGuiObject*> GuiObjects;
    ox::TArray<SLuaEntityActionButton> EntityActionButtons;
    ox::TArray<ox::video::ISpriteAnimationState*> SpriteStates;
    //! Where createSpriteState looks for a free index first.
    int FreeSpriteState;
    ox::TArray<int> RemovedSpriteStates;
};

extern CLuaManager* gp_luaManager;
extern lua_State* gp_luaState;
ox::core::CString<char> extractLuaPath(lua_State* L);

} // end namespace game
} // end namespace harvest

#endif
