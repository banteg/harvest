// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The harvest_* functions are the "harvest" Lua library; the rest is the mod and hook machinery.

#include <stdlib.h>
#include <iostream>
#include "CWorld.h"
#include "CLuaManager.h"
#include "CLuaFileValues.h"
#include "CStatistics.h"
#include "CThreatLevel.h"
#include "SInfoLineMessage.h"
#include "ox/IOxDevice.h"
#include "ox/algo/CRand.h"
#include "ox/event/IEventReceiver.h"
#include "ox/game/CConfiguration.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUIFont.h"
#include "ox/io/CHelpIO.h"
#include "ox/io/IFileList.h"
#include "ox/io/IFilePath.h"
#include "ox/io/IFileSystem.h"
#include "ox/lua/lunar.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"
#include "harvest/entity/CEntityManager.h"
#include "harvest/entity/CAlienEntity.h"
#include "harvest/entity/CBuildableItems.h"
#include "harvest/entity/CBuildingEntity.h"
#include "harvest/entity/CConstructionEntity.h"
#include "harvest/entity/CCreativeEntity.h"
#include "harvest/entity/CDropshipEntity.h"
#include "harvest/entity/CMineralsEntity.h"
#include "harvest/entity/CMissileTurretEntity.h"
#include "harvest/entity/CPerimeterBomb.h"
#include "harvest/entity/CSparkEntity.h"
#include "harvest/states/CPlayState.h"

void AlwaysAppendToFile(const char* filename, const char* format, ...);

namespace harvest {
namespace game {

ox::TArray<SLuaMod> CLuaManager::s_availableLuaScriptFiles;
lua_State* gp_luaState = 0;
CLuaManager* gp_luaManager = 0;

ox::core::CString<char> extractLuaPath(lua_State* L)
{
    lua_Debug ar;
    lua_getstack(L, 1, &ar);
    lua_getinfo(L, "S", &ar);
    ox::core::CString<char> source = ar.source;
    ox::core::CString<char> marker = "-- FILE@";

    if (source.startsWith(marker))
    {
        int end = source.findNext("\n", 0);
        if (end != -1 && end > marker.size())
        {
            source = source.subString(marker.size(), end - marker.size());
            int slash = source.findLast('/');
            if (slash != -1)
                return source.subString(0, slash + 1);
        }
    }

    source.replace('\\', '/');

    return source.subString(1, source.findLast('/'));
}

int harvest_spawnParticle(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 3)
    {
        const char* name = lua_tostring(L, 1);
        float x = (float)lua_tonumber(L, 2);
        float y = (float)lua_tonumber(L, 3);
        float z = 1.0f;
        if (top >= 4)
            z = (float)lua_tonumber(L, 4);
        ox::core::CVector3d<float> speed(0, 0, 0);
        if (top >= 6)
        {
            speed.X = (float)lua_tonumber(L, 5);
            speed.Y = (float)lua_tonumber(L, 6);
            if (top >= 7)
                speed.Z = (float)lua_tonumber(L, 7);
        }
        entity::gp_entityManager->appendEntity(new entity::CParticleEntity(x, y, z, &speed, name), 4);
    }
    return 0;
}

int harvest_spawnMinerals(lua_State* L)
{
    if (lua_gettop(L) >= 3)
    {
        float x = (float)lua_tonumber(L, 1);
        float y = (float)lua_tonumber(L, 2);
        int amount = lua_tointeger(L, 3);
        if (amount > 0)
        {
            const std::list<ox::entity::COxEntity*>& list = entity::gp_entityManager->getEntityList(0);
            for (std::list<ox::entity::COxEntity*>::const_iterator it = list.begin(); it != list.end(); ++it)
            {
                if (((entity::CEntity*)*it)->getEntityType() == 5)
                {
                    float dx = (*it)->getPosition().X - x;
                    float dy = (*it)->getPosition().Y - y;
                    if (dx * dx + dy * dy < 100.0f)
                    {
                        ((entity::CMineralsEntity*)*it)->setRemainingMinerals(
                            ((entity::CMineralsEntity*)*it)->getRemainingMinerals() + amount);
                        return 0;
                    }
                }
            }

            entity::CMineralsEntity* minerals = new entity::CMineralsEntity(x, y, 0);
            minerals->setRemainingMinerals(amount);
            entity::gp_entityManager->appendEntity(minerals, 0);
            ox::event::SEvent event;
            event.EventType = ox::event::EET_USER_EVENT;
            event.UserEvent.UserData1 = 9;
            event.UserEvent.UserData2 = 0;
            event.UserEvent.UserData3 = 0;
            event.UserEvent.UserPointer = 0;
            ox::event::gp_subscriberList->postDelayedEvent(event);
        }
    }
    return 0;
}

int harvest_findBuildings(lua_State* L)
{
    int top = lua_gettop(L);
    ox::core::CString<char> buildingId;
    int type = -1;
    if (top == 1)
    {
        buildingId = lua_tostring(L, 1);
        type = entity::gp_buildableItems->getEntityType(
            entity::gp_buildableItems->getIndexForEntityId(buildingId.c_str()));
    }
    else if (top == 4)
    {
        buildingId = lua_tostring(L, 4);
        type = entity::gp_buildableItems->getEntityType(
            entity::gp_buildableItems->getIndexForEntityId(buildingId.c_str()));
    }
    else if (top == 5)
    {
        buildingId = lua_tostring(L, 5);
        type = entity::gp_buildableItems->getEntityType(
            entity::gp_buildableItems->getIndexForEntityId(buildingId.c_str()));
    }

    float x1, y1, x2, y2;
    bool inRect, inCircle;
    if (top == 3 || top == 4)
    {
        x1 = (float)lua_tonumber(L, 1);
        y1 = (float)lua_tonumber(L, 2);
        x2 = (float)lua_tonumber(L, 3);
        x2 *= x2;
        inCircle = true;
        inRect = false;
    }
    else if (top == 5)
    {
        x1 = (float)lua_tonumber(L, 1);
        y1 = (float)lua_tonumber(L, 2);
        x2 = (float)lua_tonumber(L, 3);
        y2 = (float)lua_tonumber(L, 4);
        inCircle = false;
        inRect = true;
    }
    else
    {
        inCircle = false;
        inRect = false;
    }

    if (!entity::gp_entityManager)
        return 0;

    const std::list<ox::entity::COxEntity*>& list = entity::gp_entityManager->getEntityList(0);
    std::list<ox::entity::COxEntity*>::const_iterator it = list.begin();
    lua_newtable(L);
    int index = 1;
    for (; it != list.end(); ++it)
    {
        if (((entity::CEntity*)*it)->getEntityType() != 5)
        {
            if (type < 0 || (type == ((entity::CEntity*)*it)->getEntityType() &&
                (type != 16 || buildingId == ox::core::CString<char>(((entity::CCreativeEntity*)*it)->getBuildingId()))))
            {
                if (inRect)
                {
                    ox::core::CVector3d<float> position = (*it)->getPosition();
                    if (x1 > position.X || position.X > x2 || y1 > position.Y || position.Y > y2)
                        continue;
                }
                if (inCircle)
                {
                    const ox::core::CVector3d<float>& position = (*it)->getPosition();
                    float dx = position.X - x1;
                    float dy = position.Y - y1;
                    if (dx * dx + dy * dy > x2)
                        continue;
                }
                lua_pushnumber(L, index);
                Lunar<entity::CBuildingLuaInfo>::push(L, ((entity::CBuildingEntity*)*it)->getLuaInfo());
                lua_rawset(L, -3);
                ++index;
            }
        }
    }
    return 1;
}

int harvest_findAliens(lua_State* L)
{
    int top = lua_gettop(L);
    int alienType = -1;
    if (top == 1)
        alienType = lua_tointeger(L, 1);
    else if (top == 4)
        alienType = lua_tointeger(L, 4);
    else if (top == 5)
        alienType = lua_tointeger(L, 5);

    float x1, y1, x2, y2;
    bool inRect, inCircle;
    if (top == 3 || top == 4)
    {
        x1 = (float)lua_tonumber(L, 1);
        y1 = (float)lua_tonumber(L, 2);
        x2 = (float)lua_tonumber(L, 3);
        x2 *= x2;
        inCircle = true;
        inRect = false;
    }
    else if (top == 5)
    {
        x1 = (float)lua_tonumber(L, 1);
        y1 = (float)lua_tonumber(L, 2);
        x2 = (float)lua_tonumber(L, 3);
        y2 = (float)lua_tonumber(L, 4);
        inCircle = false;
        inRect = true;
    }
    else
    {
        inCircle = false;
        inRect = false;
    }

    if (!entity::gp_entityManager)
        return 0;

    const std::list<ox::entity::COxEntity*>& list = entity::gp_entityManager->getEntityList(1);
    std::list<ox::entity::COxEntity*>::const_iterator it = list.begin();
    lua_newtable(L);
    int index = 1;
    for (; it != list.end(); ++it)
    {
        if (alienType >= 0 && alienType != ((entity::CAlienEntity*)*it)->getAlienType())
            continue;
        if (inRect)
        {
            ox::core::CVector3d<float> position = (*it)->getPosition();
            if (x1 > position.X || position.X > x2 || y1 > position.Y || position.Y > y2)
                continue;
        }
        if (inCircle)
        {
            const ox::core::CVector3d<float>& position = (*it)->getPosition();
            float dx = position.X - x1;
            float dy = position.Y - y1;
            if (dx * dx + dy * dy > x2)
                continue;
        }
        lua_pushnumber(L, index);
        Lunar<entity::CAlienLuaInfo>::push(L, ((entity::CAlienEntity*)*it)->getLuaInfo());
        lua_rawset(L, -3);
        ++index;
    }
    return 1;
}

int harvest_getAlien(lua_State* L)
{
    if (lua_gettop(L) > 0)
    {
        int id = lua_tointeger(L, 1);
        entity::CAlienEntity* alien = (entity::CAlienEntity*)entity::gp_entityManager->locateEntity(id, 1);
        if (alien)
        {
            Lunar<entity::CAlienLuaInfo>::push(L, alien->getLuaInfo());
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}

int harvest_getBuilding(lua_State* L)
{
    if (lua_gettop(L) > 0)
    {
        int id = lua_tointeger(L, 1);
        entity::CBuildingEntity* building = (entity::CBuildingEntity*)entity::gp_entityManager->locateEntity(id, 0);
        if (building)
        {
            Lunar<entity::CBuildingLuaInfo>::push(L, building->getLuaInfo());
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}

int harvest_setCreativeListVisible(lua_State* L)
{
    if (lua_gettop(L) > 0)
    {
        bool visible = lua_toboolean(L, 1) != 0;
        gp_luaManager->CreativeListVisible = visible;
        ox::event::SEvent event;
        event.EventType = ox::event::EET_USER_EVENT;
        event.UserEvent.UserData1 = 1;
        event.UserEvent.UserData2 = 0;
        event.UserEvent.UserData3 = 0;
        event.UserEvent.UserPointer = 0;
        ox::event::gp_subscriberList->OnEvent(event);
    }
    return 0;
}

void CLuaManager::setCreativeListVisible(bool visible)
{
    CreativeListVisible = visible;
    ox::event::SEvent event;
    event.EventType = ox::event::EET_USER_EVENT;
    event.UserEvent.UserData1 = 1;
    event.UserEvent.UserData2 = 0;
    event.UserEvent.UserData3 = 0;
    event.UserEvent.UserPointer = 0;
    ox::event::gp_subscriberList->OnEvent(event);
}

int harvest_setRushListVisible(lua_State* L)
{
    if (lua_gettop(L) > 0)
    {
        bool visible = lua_toboolean(L, 1) != 0;
        gp_luaManager->RushListVisible = visible;
        ox::event::SEvent event;
        event.EventType = ox::event::EET_USER_EVENT;
        event.UserEvent.UserData1 = 1;
        event.UserEvent.UserData2 = 0;
        event.UserEvent.UserData3 = 0;
        event.UserEvent.UserPointer = 0;
        ox::event::gp_subscriberList->OnEvent(event);
    }
    return 0;
}

void CLuaManager::setRushListVisible(bool visible)
{
    RushListVisible = visible;
    ox::event::SEvent event;
    event.EventType = ox::event::EET_USER_EVENT;
    event.UserEvent.UserData1 = 1;
    event.UserEvent.UserData2 = 0;
    event.UserEvent.UserData3 = 0;
    event.UserEvent.UserPointer = 0;
    ox::event::gp_subscriberList->OnEvent(event);
}

int harvest_setRushProgress(lua_State* L)
{
    if (lua_gettop(L) > 0)
    {
        float progress = (float)lua_tonumber(L, 1);
        gp_luaManager->RushProgress = ox::core::clamp(progress, 0.0f, 1.0f);
    }
    return 0;
}

void CLuaManager::setRushProgress(float progress)
{
    RushProgress = ox::core::clamp(progress, 0.0f, 1.0f);
}

int harvest_setWaveListVisible(lua_State* L)
{
    if (lua_gettop(L) > 0)
    {
        bool visible = lua_toboolean(L, 1) != 0;
        gp_luaManager->WaveListVisible = visible;
        ox::event::SEvent event;
        event.EventType = ox::event::EET_USER_EVENT;
        event.UserEvent.UserData1 = 1;
        event.UserEvent.UserData2 = 0;
        event.UserEvent.UserData3 = 0;
        event.UserEvent.UserPointer = 0;
        ox::event::gp_subscriberList->OnEvent(event);
    }
    return 0;
}

void CLuaManager::setWaveListVisible(bool visible)
{
    WaveListVisible = visible;
    ox::event::SEvent event;
    event.EventType = ox::event::EET_USER_EVENT;
    event.UserEvent.UserData1 = 1;
    event.UserEvent.UserData2 = 0;
    event.UserEvent.UserData3 = 0;
    event.UserEvent.UserPointer = 0;
    ox::event::gp_subscriberList->OnEvent(event);
}

int harvest_setTimerVisible(lua_State* L)
{
    if (lua_gettop(L) > 0)
    {
        bool visible = lua_toboolean(L, 1) != 0;
        gp_luaManager->TimerVisible = visible;
        ox::event::SEvent event;
        event.EventType = ox::event::EET_USER_EVENT;
        event.UserEvent.UserData1 = 1;
        event.UserEvent.UserData2 = 0;
        event.UserEvent.UserData3 = 0;
        event.UserEvent.UserPointer = 0;
        ox::event::gp_subscriberList->OnEvent(event);
    }
    return 0;
}

void CLuaManager::setTimerVisible(bool visible)
{
    TimerVisible = visible;
    ox::event::SEvent event;
    event.EventType = ox::event::EET_USER_EVENT;
    event.UserEvent.UserData1 = 1;
    event.UserEvent.UserData2 = 0;
    event.UserEvent.UserData3 = 0;
    event.UserEvent.UserPointer = 0;
    ox::event::gp_subscriberList->OnEvent(event);
}

int harvest_setTimerValue(lua_State* L)
{
    if (lua_gettop(L) > 0)
    {
        float value = ox::core::max_(0.0f, (float)lua_tonumber(L, 1));
        gp_luaManager->TimerValue = value;
    }
    return 0;
}

void CLuaManager::setTimerValue(float value)
{
    TimerValue = ox::core::max_(0.0f, value);
}

int harvest_setWaveButtonVisible(lua_State* L)
{
    if (lua_gettop(L) >= 2)
        gp_luaManager->setWaveButtonVisible(lua_tointeger(L, 1) - 1, lua_toboolean(L, 2) != 0);
    return 0;
}

void CLuaManager::setWaveButtonVisible(int button, bool visible)
{
    if ((unsigned int)button > 9)
        return;
    WaveButtonVisible[button] = visible;
    ox::event::SEvent event;
    event.EventType = ox::event::EET_USER_EVENT;
    event.UserEvent.UserData1 = 1;
    event.UserEvent.UserData2 = 0;
    event.UserEvent.UserData3 = 0;
    event.UserEvent.UserPointer = 0;
    ox::event::gp_subscriberList->OnEvent(event);
}

int harvest_setWaveButtonAliens(lua_State* L)
{
    if (lua_gettop(L) > 0)
    {
        bool aliens[14];
        for (int i = 0; i < 14; ++i)
            aliens[i] = false;
        for (int i = 2; i <= lua_gettop(L); ++i)
        {
            int alien = lua_tointeger(L, i);
            if (alien > 0 && alien - 1 < 14)
                aliens[alien - 1] = true;
        }
        gp_luaManager->setWaveButtonAliens(lua_tointeger(L, 1) - 1, aliens);
    }
    return 0;
}

void CLuaManager::setWaveButtonAliens(int button, bool* aliens)
{
    if ((unsigned int)button > 9)
        return;
    for (int i = 0; i < 14; ++i)
        WaveButtonAliens[button][i] = aliens[i];
}

int harvest_setWaveButtonLabel(lua_State* L)
{
    if (lua_gettop(L) >= 2)
        gp_luaManager->setWaveButtonNumber(lua_tointeger(L, 1) - 1, lua_tointeger(L, 2));
    return 0;
}

void CLuaManager::setWaveButtonNumber(int button, int number)
{
    if ((unsigned int)button <= 9)
        WaveButtonNumber[button] = number;
}

int harvest_showInfoMessage(lua_State* L)
{
    int top = lua_gettop(L);
    if (top > 0)
    {
        SInfoLineMessage message;
        message.Text = lua_tostring(L, 1);
        if (top >= 2)
            message.Name = lua_tostring(L, 2);
        if (top >= 3)
            message.Portrait = lua_tostring(L, 3);
        if (top >= 4)
            message.Sound = lua_tostring(L, 4);
        ox::event::SEvent event;
        event.EventType = ox::event::EET_USER_EVENT;
        event.UserEvent.UserData1 = 32;
        event.UserEvent.UserPointer = &message;
        gp_luaManager->EventReceiver->OnEvent(event);
    }
    return 0;
}

int harvest_getNumAliens(lua_State* L)
{
    if (!entity::gp_entityManager)
        return 0;
    lua_pushinteger(L, entity::gp_entityManager->getNumAliens());
    return 1;
}

int harvest_getNumBuildings(lua_State* L)
{
    if (!entity::gp_entityManager)
        return 0;
    lua_pushinteger(L, entity::gp_entityManager->getNumBuildings());
    return 1;
}

int harvest_winGame(lua_State* L)
{
    ox::event::SEvent event;
    event.EventType = ox::event::EET_USER_EVENT;
    event.UserEvent.UserData1 = 26;
    event.UserEvent.UserData2 = 0;
    event.UserEvent.UserData3 = 0;
    event.UserEvent.UserPointer = 0;
    ox::event::gp_subscriberList->OnEvent(event);
    return 0;
}

int harvest_loseGame(lua_State* L)
{
    ox::event::SEvent event;
    event.EventType = ox::event::EET_USER_EVENT;
    event.UserEvent.UserData1 = 27;
    event.UserEvent.UserData2 = 0;
    event.UserEvent.UserData3 = 0;
    event.UserEvent.UserPointer = 0;
    ox::event::gp_subscriberList->OnEvent(event);
    return 0;
}

int harvest_setMinimumWorldBorders(lua_State* L)
{
    if (lua_gettop(L) >= 4)
    {
        float left = (float)lua_tonumber(L, 1);
        float top = (float)lua_tonumber(L, 2);
        float right = (float)lua_tonumber(L, 3);
        float bottom = (float)lua_tonumber(L, 4);
        left = ox::core::clamp(left, -4096.0f, right);
        right = ox::core::clamp(right, left, 5120.0f);
        top = ox::core::clamp(top, -4096.0f, bottom);
        bottom = ox::core::clamp(bottom, top, 5120.0f);
        gp_luaManager->setMinimumWorldBorders(ox::core::CRect<float>(left, top, right, bottom));
    }
    return 0;
}

void CLuaManager::setMinimumWorldBorders(const ox::core::CRect<float>& borders)
{
    MinimumWorldBorders = borders;
}

int harvest_getTotalAlienDamage(lua_State* L)
{
    if (!gp_statistics)
        return 0;
    lua_pushnumber(L, gp_statistics->getRushModeDamage());
    return 1;
}

int harvest_dofile(lua_State* L)
{
    ox::core::CString<char> filename = extractLuaPath(L);
    filename.append(ox::core::CString<char>(luaL_optstring(L, 1, 0)));
    int top = lua_gettop(L);
    if (gp_luaManager->getFileSystem()->existFile(filename.c_str(), false) == true)
    {
        ox::io::IFilePath* path = gp_luaManager->getFileSystem()->resolveAliases(filename.c_str());
        if (path)
        {
            if (luaL_loadfile(L, path->getPath()) != 0)
                lua_error(L);
            lua_call(L, 0, LUA_MULTRET);
            path->drop();
        }
    }
    else
    {
        ox::io::IReadFile* file = gp_luaManager->getFileSystem()->createAndOpenFile(filename.c_str());
        if (file)
        {
            ox::core::CString<char> script = "-- FILE@";
            script.append(filename);
            script.append(ox::core::CString<char>("\n"));
            ox::core::CString<char> content;
            ox::io::CHelpIO::readString(file, content);
            script.append(content);
            if (luaL_loadstring(L, script.c_str()) != 0 || lua_pcall(L, 0, LUA_MULTRET, 0) != 0)
                lua_tostring(L, -1);
        }
    }
    return lua_gettop(L) - top;
}

ox::io::IFileSystem* CLuaManager::getFileSystem()
{
    if (Device && Device->getFileSystem())
        return Device->getFileSystem();
    return 0;
}

int harvest_addSpriteState(lua_State* L)
{
    if (lua_gettop(L) <= 0)
    {
        lua_pushinteger(L, -1);
        return 1;
    }
    ox::core::CString<char> packageFile;
    if (lua_gettop(L) >= 2)
    {
        packageFile = extractLuaPath(L);
        packageFile.append(ox::core::CString<char>(lua_tostring(L, 2)));
    }
    lua_pushinteger(L, gp_luaManager->createSpriteState(lua_tostring(L, 1), packageFile));
    return 1;
}

int CLuaManager::createSpriteState(const char* name, const ox::core::CString<char>& packageFile)
{
    ox::video::ISpritePackage* package = entity::CEntity::gp_spritePackage;
    if (packageFile.size() >= 5)
    {
        package = Device->getVideoDriver()->getSpritePackage(packageFile.c_str(), false);
        if (!package)
            return -1;
    }

    ox::video::ISpriteAnimationState* state = package->addNewAnimationState(ox::core::CString<char>(name));
    if (!state)
        return -1;

    if (FreeSpriteState < (int)SpriteStates.size())
    {
        int index = FreeSpriteState++;
        SpriteStates[index] = state;
        while (FreeSpriteState < (int)SpriteStates.size())
        {
            if (!SpriteStates[FreeSpriteState])
                break;
            ++FreeSpriteState;
        }
        return index;
    }

    SpriteStates.push_back(state);
    FreeSpriteState = SpriteStates.size();
    return FreeSpriteState - 1;
}

int harvest_removeSpriteState(lua_State* L)
{
    if (lua_gettop(L) > 0)
        gp_luaManager->markSpriteState(lua_tointeger(L, 1));
    return 0;
}

void CLuaManager::markSpriteState(int index)
{
    RemovedSpriteStates.push_back(index);
}

int harvest_renderSpriteState(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 3)
    {
        int index = lua_tointeger(L, 1);
        lua_Number x = lua_tonumber(L, 2);
        lua_Number y = lua_tonumber(L, 3);
        float z = 0.0f;
        float scale = 1.0f;
        float rotation = 0.0f;
        ox::video::SColor color(0xffffffff);
        if (top >= 4)
            z = (float)lua_tonumber(L, 4);
        if (top >= 5)
            scale = (float)lua_tonumber(L, 5);
        if (top >= 6)
            rotation = (float)lua_tonumber(L, 6);
        if (top >= 7)
            color.setAlpha(lua_tointeger(L, 7));
        if (top >= 10)
        {
            color.setRed(lua_tointeger(L, 8));
            color.setGreen(lua_tointeger(L, 9));
            color.setBlue(lua_tointeger(L, 10));
        }
        entity::CSpecialEffectEntity* effect = new entity::CSpecialEffectEntity((float)x, (float)y, z,
            gp_luaManager->getSpriteState(index), scale, rotation, color);
        entity::gp_entityManager->appendEntity(effect, 4);
    }
    return 0;
}

ox::video::ISpriteAnimationState* CLuaManager::getSpriteState(int index)
{
    if (index >= 0 && index < (int)SpriteStates.size())
        return SpriteStates[index];
    return 0;
}

int harvest_renderSpriteStateFreeShape(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 10)
    {
        int index = lua_tointeger(L, 1);
        lua_Number x1 = lua_tonumber(L, 2);
        lua_Number y1 = lua_tonumber(L, 3);
        lua_Number x2 = lua_tonumber(L, 4);
        lua_Number y2 = lua_tonumber(L, 5);
        lua_Number x3 = lua_tonumber(L, 6);
        lua_Number y3 = lua_tonumber(L, 7);
        lua_Number x4 = lua_tonumber(L, 8);
        lua_Number y4 = lua_tonumber(L, 9);
        //! The entity's y, which orders it among the other entities.
        lua_Number y = lua_tonumber(L, 10);
        ox::video::SColor color(0xffffffff);
        if (top >= 11)
            color.setAlpha(lua_tointeger(L, 11));
        if (top >= 14)
        {
            color.setRed(lua_tointeger(L, 12));
            color.setGreen(lua_tointeger(L, 13));
            color.setBlue(lua_tointeger(L, 14));
        }
        ox::video::ISpriteAnimationState* sprite = gp_luaManager->getSpriteState(index);
        entity::CSpecialEffectEntity* effect = new entity::CSpecialEffectEntity(
            ox::core::CPosition2d<float>((float)x1, (float)y1), ox::core::CPosition2d<float>((float)x2, (float)y2),
            ox::core::CPosition2d<float>((float)x3, (float)y3), ox::core::CPosition2d<float>((float)x4, (float)y4),
            (float)y, sprite, color);
        entity::gp_entityManager->appendEntity(effect, 4);
    }
    return 0;
}

int harvest_getViewPosition(lua_State* L)
{
    lua_pushnumber(L, gp_luaManager->getViewPosition().X);
    lua_pushnumber(L, gp_luaManager->getViewPosition().Y);
    return 2;
}

const ox::core::CPosition2d<float>& CLuaManager::getViewPosition() const
{
    return ViewPosition;
}

int harvest_setViewPosition(lua_State* L)
{
    if (lua_gettop(L) >= 2)
    {
        int x = (int)lua_tonumber(L, 1);
        int y = (int)lua_tonumber(L, 2);
        gp_luaManager->setViewPosition(ox::core::CPosition2d<float>((float)x, (float)y));
        ox::event::SEvent event;
        event.EventType = ox::event::EET_USER_EVENT;
        event.UserEvent.UserData1 = 30;
        event.UserEvent.UserData2 = x;
        event.UserEvent.UserData3 = y;
        event.UserEvent.UserPointer = 0;
        ox::event::gp_subscriberList->OnEvent(event);
    }
    return 0;
}

void CLuaManager::setViewPosition(const ox::core::CPosition2d<float>& position)
{
    ViewPosition = position;
}

int harvest_isKeyPressed(lua_State* L)
{
    bool pressed = false;
    if (lua_gettop(L) > 0)
    {
        int key = lua_tointeger(L, 1);
        pressed = key >= 0 && key <= 255 && states::CPlayState::m_keys[key];
    }
    lua_pushboolean(L, pressed);
    return 1;
}

int harvest_getMousePosition(lua_State* L)
{
    ox::core::CPosition2d<int> position = gp_luaManager->getMousePosition();
    lua_pushinteger(L, position.X);
    lua_pushinteger(L, position.Y);
    return 2;
}

ox::core::CPosition2d<int> CLuaManager::getMousePosition()
{
    if (Device && Device->getGUIEnvironment())
        return Device->getGUIEnvironment()->getMousePosition();
    return ox::core::CPosition2d<int>(0, 0);
}

int harvest_setBuildingEnabled(lua_State* L)
{
    if (lua_gettop(L) >= 2)
    {
        ox::core::CString<char> buildingId = lua_tostring(L, 1);
        bool enabled = lua_toboolean(L, 2) != 0;
        entity::SBuildingInfoItem* info = entity::gp_buildableItems->getBuildingInfoByEntityId(buildingId.c_str());
        if (info && info->Enabled != enabled)
        {
            info->Enabled = enabled;
            ox::event::SEvent event;
            event.EventType = ox::event::EET_USER_EVENT;
            event.UserEvent.UserData1 = 1;
            event.UserEvent.UserData2 = 0;
            event.UserEvent.UserData3 = 0;
            event.UserEvent.UserPointer = 0;
            ox::event::gp_subscriberList->OnEvent(event);
        }
    }
    return 0;
}

int harvest_defineUpgrade(lua_State* L)
{
    if (lua_gettop(L) >= 7 && entity::gp_buildableItems)
    {
        ox::core::CString<char> entityId = lua_tostring(L, 1);
        ox::core::CString<char> upgradeId = lua_tostring(L, 2);
        ox::core::CString<char> name = lua_tostring(L, 3);
        int cost = lua_tointeger(L, 4);
        int count = lua_tointeger(L, 5);
        float value = (float)lua_tonumber(L, 6);
        ox::core::CString<char> description = lua_tostring(L, 7);
        entity::gp_buildableItems->addSpecialUpgrade(entityId.c_str(), upgradeId.c_str(), name.c_str(),
            cost, ox::core::max_(count, 1), value, description.c_str());
    }
    return 0;
}

int harvest_defineActionButton(lua_State* L)
{
    if (lua_gettop(L) >= 2)
        gp_luaManager->addEntityActionButton(lua_tostring(L, 1), lua_tostring(L, 2), false);
    return 0;
}

void CLuaManager::addEntityActionButton(const char* entityId, const char* command, bool upgrade)
{
    SLuaEntityActionButton button;
    button.Command = command;
    button.Id = -1;
    button.EntityId = entityId;
    button.Upgrade = upgrade;
    EntityActionButtons.push_back(button);
}

int harvest_defineUpgradeButton(lua_State* L)
{
    if (lua_gettop(L) >= 2)
        gp_luaManager->addEntityActionButton(lua_tostring(L, 1), lua_tostring(L, 2), true);
    return 0;
}

int harvest_spawnEnergySpark(lua_State* L)
{
    if (lua_gettop(L) > 0)
    {
        int id = lua_tointeger(L, 1);
        entity::CEntity* source = (entity::CEntity*)entity::gp_entityManager->locateEntity(id, 0);
        if (source)
        {
            int index = ox::algo::CRand::rand();
            int target = source->findSparkTarget(-1, index);
            float x = source->getPosition().X;
            float y = source->getPosition().Y;
            entity::gp_entityManager->appendEntity(new entity::CSparkEntity(x, y, target, id), 2);
        }
    }
    return 0;
}

int harvest_spawnBullet(lua_State* L)
{
    if (lua_gettop(L) >= 7)
    {
        lua_Number damage = lua_tonumber(L, 1);
        lua_Number startX = lua_tonumber(L, 2);
        lua_Number startY = lua_tonumber(L, 3);
        lua_Number startZ = lua_tonumber(L, 4);
        lua_Number targetX = lua_tonumber(L, 5);
        lua_Number targetY = lua_tonumber(L, 6);
        lua_Number targetZ = lua_tonumber(L, 7);
        ox::core::CVector3d<float> start((float)startX, (float)startY, (float)startZ);
        ox::core::CVector3d<float> target((float)targetX, (float)targetY, (float)targetZ);
        entity::CDropshipBulletEntity* bullet = new entity::CDropshipBulletEntity((float)damage, start, target);
        entity::gp_entityManager->appendEntity(bullet, 4);
    }
    return 0;
}

int harvest_getSelectedBuilding(lua_State* L)
{
    entity::CEntity* building = gp_luaManager->getSelectedBuilding();
    if (!building)
    {
        ox::TArray<ox::entity::SEntityReference*>* buildings = gp_luaManager->getSelectedBuildings();
        if (!buildings || buildings->empty())
        {
            lua_pushnil(L);
            return 1;
        }
        ox::TArray<ox::entity::SEntityReference*>::iterator it = buildings->begin();
        while (true)
        {
            if (it == buildings->end())
            {
                lua_pushnil(L);
                return 1;
            }
            if (((entity::CEntity*)(*it)->Entity)->getEntityType() != 5)
                break;
            ++it;
        }
        building = (entity::CEntity*)(*it)->Entity;
    }
    Lunar<entity::CBuildingLuaInfo>::push(L, ((entity::CBuildingEntity*)building)->getLuaInfo());
    return 1;
}

entity::CEntity* CLuaManager::getSelectedBuilding()
{
    return SelectedBuilding;
}

ox::TArray<ox::entity::SEntityReference*>* CLuaManager::getSelectedBuildings()
{
    return SelectedBuildings;
}

int harvest_getSelectedBuildings(lua_State* L)
{
    ox::TArray<ox::entity::SEntityReference*>* buildings = gp_luaManager->getSelectedBuildings();
    if (!buildings || buildings->empty())
    {
        entity::CEntity* building = gp_luaManager->getSelectedBuilding();
        if (!building)
            return 0;
        lua_newtable(L);
        lua_pushnumber(L, 1);
        Lunar<entity::CBuildingLuaInfo>::push(L, ((entity::CBuildingEntity*)building)->getLuaInfo());
        lua_rawset(L, -3);
        return 1;
    }

    lua_newtable(L);
    int index = 1;
    for (ox::TArray<ox::entity::SEntityReference*>::iterator it = buildings->begin();
         it != buildings->end(); ++it)
    {
        if (((entity::CEntity*)(*it)->Entity)->getEntityType() != 5)
        {
            lua_pushnumber(L, index++);
            Lunar<entity::CBuildingLuaInfo>::push(L, ((entity::CBuildingEntity*)(*it)->Entity)->getLuaInfo());
            lua_rawset(L, -3);
        }
    }
    return 1;
}

int harvest_print(lua_State* L)
{
    if (lua_gettop(L) > 0)
        CLuaManager::postStringAsInfo(ox::core::CString<char>(lua_tostring(L, 1)));
    return 0;
}

int harvest_setThreatLevelProgress(lua_State* L)
{
    if (gp_luaManager && lua_gettop(L) > 0)
        gp_luaManager->setThreatLevelProgress((float)lua_tonumber(L, 1));
    return 0;
}

int harvest_setThreatLevelValue(lua_State* L)
{
    if (gp_luaManager && lua_gettop(L) > 0)
        gp_luaManager->setThreatLevelValue((int)lua_tonumber(L, 1));
    return 0;
}

int harvest_getCredits(lua_State* L)
{
    if (gp_mineralAmount)
        lua_pushinteger(L, gp_mineralAmount->getValue());
    else
        lua_pushinteger(L, 0);
    return 1;
}

int harvest_addCredits(lua_State* L)
{
    if (gp_mineralAmount && lua_gettop(L) > 0)
    {
        float credits = (float)lua_tonumber(L, 1);
        int amount = (int)(credits >= 0 ? credits + 0.5f : credits - 0.5f);
        gp_mineralAmount->modifyValue(amount, 0, 9999999);
        gp_negatedMineralAmount->modifyValue(-amount, -9999999, 0);
    }
    return 0;
}

int harvest_getPlanet(lua_State* L)
{
    if (!gp_world)
        return 0;
    switch (gp_world->getPlanet())
    {
    case 2:
        lua_pushstring(L, "ares");
        return 1;
    case 1:
        lua_pushstring(L, "pose");
        return 1;
    case 0:
        lua_pushstring(L, "heph");
        return 1;
    }
    return 1;
}

int harvest_getWorldBorders(lua_State* L)
{
    if (!gp_world)
        return 0;
    ox::core::CRect<float> borders = gp_world->getVisibleGameFieldSize();
    lua_pushnumber(L, borders.UpperLeftCorner.X);
    lua_pushnumber(L, borders.UpperLeftCorner.Y);
    lua_pushnumber(L, borders.LowerRightCorner.X);
    lua_pushnumber(L, borders.LowerRightCorner.Y);
    return 4;
}

int harvest_isAlienAvailable(lua_State* L)
{
    if (!gp_world || lua_gettop(L) <= 0)
        return 0;
    int alien = lua_tointeger(L, 1);
    lua_pushboolean(L, CThreatLevel::alienOccursOnPlanet(gp_world->getPlanet(), alien - 1));
    return 1;
}

int harvest_isPositionBlocked(lua_State* L)
{
    if (!gp_world || lua_gettop(L) < 2)
        return 0;
    ox::core::CPosition2d<float> position;
    position.X = (float)lua_tonumber(L, 1);
    position.Y = (float)lua_tonumber(L, 2);
    bool free = gp_world->mayPlaceObjectHere(position, false);
    if (free && lua_gettop(L) > 2)
    {
        entity::SBuildingInfoItem* info =
            entity::gp_buildableItems->getBuildingInfoByEntityId(lua_tostring(L, 3));
        if (info)
            free = !entity::gp_entityManager->isBuildingPlacementOk(position, info->CollisionSize);
    }
    lua_pushboolean(L, !free);
    return 1;
}

int harvest_spawnAlien(lua_State* L)
{
    if (gp_world && lua_gettop(L) >= 3 && entity::gp_entityManager)
    {
        float type = (float)lua_tonumber(L, 1);
        int alienType = (int)(type >= 0 ? type + 0.5f : type - 0.5f);
        alienType -= 1;
        float x = (float)lua_tonumber(L, 2);
        float y = (float)lua_tonumber(L, 3);
        if (CThreatLevel::alienOccursOnPlanet(gp_world->getPlanet(), alienType))
            entity::gp_entityManager->addAlien(alienType, x, y);
    }
    return 0;
}

int harvest_spawnBuilding(lua_State* L)
{
    if (!gp_world || lua_gettop(L) < 3 || !entity::gp_entityManager)
        return 0;
    const char* buildingId = lua_tostring(L, 1);
    float x = (float)lua_tonumber(L, 2);
    float y = (float)lua_tonumber(L, 3);
    float progress = 1.0f;
    if (lua_gettop(L) >= 4)
        progress = (float)lua_tonumber(L, 4);
    if (entity::gp_buildableItems->getIndexForEntityId(buildingId) == -1)
    {
        lua_pushinteger(L, -1);
        return 1;
    }
    entity::CConstructionEntity* construction = new entity::CConstructionEntity(x, y, buildingId);
    if (!construction)
    {
        lua_pushinteger(L, -1);
        return 1;
    }
    construction->setProgress(progress);
    entity::gp_entityManager->appendEntity(construction, 0);
    lua_pushinteger(L, construction->getId());
    return 1;
}

int harvest_spawnChargeBomb(lua_State* L)
{
    if (gp_world && lua_gettop(L) >= 2 && entity::gp_entityManager)
    {
        double x = lua_tonumber(L, 1);
        double y = lua_tonumber(L, 2);
        float remaining = 1.0f;
        if (lua_gettop(L) >= 3)
            remaining = 1.0f - (float)lua_tonumber(L, 3);
        entity::CPerimeterBombExplosion* explosion = new entity::CPerimeterBombExplosion((float)x, (float)y);
        explosion->setFuse(remaining * 3.0f);
        entity::gp_entityManager->appendEntity(explosion, 3);
    }
    return 0;
}

int harvest_spawnMissile(lua_State* L)
{
    if (gp_world && lua_gettop(L) >= 4 && entity::gp_entityManager)
    {
        double x = lua_tonumber(L, 1);
        double y = lua_tonumber(L, 2);
        double targetX = lua_tonumber(L, 3);
        double targetY = lua_tonumber(L, 4);
        ox::core::CPosition2d<float> target((float)targetX, (float)targetY);
        entity::CMissileEntity* missile = new entity::CMissileEntity((float)x, (float)y, target, -1, 0, -1);
        entity::gp_entityManager->appendEntity(missile, 3);
    }
    return 0;
}

int harvest_spawnEagleMissile(lua_State* L)
{
    if (gp_world && lua_gettop(L) >= 2 && entity::gp_entityManager)
    {
        float x = (float)lua_tonumber(L, 1);
        float y = (float)lua_tonumber(L, 2);
        float z = 0;
        if (lua_gettop(L) >= 3)
            z = (float)lua_tonumber(L, 3);
        int top = lua_gettop(L);
        int target = -1;
        if (top >= 4)
            target = lua_tointeger(L, 4);
        if (top < 4 || target <= 0)
        {
            entity::CAlienEntity* alien = entity::CMissileTurretEntity::findPriorityAlien(
                ox::core::CVector3d<float>(x, y, 0), 3, 2250000.0f, 900.0f, 0);
            if (alien)
                target = alien->getId();
        }
        entity::CMissileEntity* missile = new entity::CMissileEntity(x, y,
            ox::core::CPosition2d<float>(0, 0), -1, 2, target);
        missile->setPosition(x, y, z);
        entity::gp_entityManager->appendEntity(missile, 3);
    }
    return 0;
}

int harvest_spawnTempestMissile(lua_State* L)
{
    if (gp_world && lua_gettop(L) >= 4 && entity::gp_entityManager)
    {
        double x = lua_tonumber(L, 1);
        double y = lua_tonumber(L, 2);
        double targetX = lua_tonumber(L, 3);
        double targetY = lua_tonumber(L, 4);
        ox::core::CPosition2d<float> target((float)targetX, (float)targetY);
        entity::CMissileEntity* missile = new entity::CMissileEntity((float)x, (float)y, target, -1, 1, -1);
        entity::gp_entityManager->appendEntity(missile, 3);
    }
    return 0;
}

int harvest_removeMinerals(lua_State* L)
{
    if (!entity::gp_entityManager)
        return 0;
    switch (lua_gettop(L))
    {
    case 0:
    {
        const std::list<ox::entity::COxEntity*>& list = entity::gp_entityManager->getEntityList(0);
        for (std::list<ox::entity::COxEntity*>::const_iterator it = list.begin(); it != list.end(); ++it)
            if (((entity::CEntity*)*it)->getEntityType() == 5)
                (*it)->killEntity();
        break;
    }
    case 1:
    {
        int id = lua_tointeger(L, 1);
        entity::CEntity* minerals = (entity::CEntity*)entity::gp_entityManager->locateEntity(id, 0);
        if (minerals && minerals->getEntityType() == 5)
            minerals->killEntity();
        break;
    }
    case 2:
    {
        int id = lua_tointeger(L, 1);
        int amount = lua_tointeger(L, 2);
        entity::CMineralsEntity* minerals = (entity::CMineralsEntity*)entity::gp_entityManager->locateEntity(id, 0);
        if (minerals && minerals->getEntityType() == 5)
            minerals->withdrawAmount(amount);
        break;
    }
    case 3:
    {
        float x = (float)lua_tonumber(L, 1);
        float y = (float)lua_tonumber(L, 2);
        float radius = (float)lua_tonumber(L, 3);
        radius *= radius;
        const std::list<ox::entity::COxEntity*>& list = entity::gp_entityManager->getEntityList(0);
        for (std::list<ox::entity::COxEntity*>::const_iterator it = list.begin(); it != list.end(); ++it)
        {
            if (((entity::CEntity*)*it)->getEntityType() == 5)
            {
                float dx = (*it)->getPosition().X - x;
                float dy = (*it)->getPosition().Y - y;
                if (dx * dx + dy * dy < radius)
                    (*it)->killEntity();
            }
        }
        break;
    }
    case 4:
    {
        float x = (float)lua_tonumber(L, 1);
        float y = (float)lua_tonumber(L, 2);
        float radius = (float)lua_tonumber(L, 3);
        int amount = lua_tointeger(L, 4);
        radius *= radius;
        const std::list<ox::entity::COxEntity*>& list = entity::gp_entityManager->getEntityList(0);
        for (std::list<ox::entity::COxEntity*>::const_iterator it = list.begin(); it != list.end(); ++it)
        {
            if (((entity::CEntity*)*it)->getEntityType() == 5)
            {
                float dx = (*it)->getPosition().X - x;
                float dy = (*it)->getPosition().Y - y;
                if (dx * dx + dy * dy < radius)
                    ((entity::CMineralsEntity*)*it)->withdrawAmount(amount);
            }
        }
        break;
    }
    case 5:
    {
        float left = (float)lua_tonumber(L, 1);
        float top = (float)lua_tonumber(L, 2);
        float right = (float)lua_tonumber(L, 3);
        float bottom = (float)lua_tonumber(L, 4);
        int amount = lua_tointeger(L, 5);
        const std::list<ox::entity::COxEntity*>& list = entity::gp_entityManager->getEntityList(0);
        for (std::list<ox::entity::COxEntity*>::const_iterator it = list.begin(); it != list.end(); ++it)
        {
            if (((entity::CEntity*)*it)->getEntityType() == 5)
            {
                float x = (*it)->getPosition().X;
                float y = (*it)->getPosition().Y;
                if (x >= left && right >= x && y >= top && bottom >= y)
                    ((entity::CMineralsEntity*)*it)->withdrawAmount(amount);
            }
        }
        break;
    }
    }
    return 0;
}

int harvest_screenToWorldCoordinates(lua_State* L)
{
    ox::core::CPosition2d<float> position(0, 0);
    if (gp_luaManager && lua_gettop(L) >= 2)
    {
        int x = lua_tointeger(L, 1);
        int y = lua_tointeger(L, 2);
        position = gp_luaManager->screenToWorld(ox::core::CPosition2d<int>(x, y));
    }
    lua_pushnumber(L, position.X);
    lua_pushnumber(L, position.Y);
    return 2;
}

int harvest_worldToScreenCoordinates(lua_State* L)
{
    ox::core::CPosition2d<int> position(0, 0);
    if (gp_luaManager && lua_gettop(L) >= 2)
    {
        float x = (float)lua_tonumber(L, 1);
        float y = (float)lua_tonumber(L, 2);
        position = gp_luaManager->worldToScreen(ox::core::CPosition2d<float>(x, y));
    }
    lua_pushinteger(L, position.X);
    lua_pushinteger(L, position.Y);
    return 2;
}

int harvest_getScreenSize(lua_State* L)
{
    if (!entity::CEntity::gp_videoDriver)
        return 0;
    ox::core::CDimension2d<int> size = entity::CEntity::gp_videoDriver->getScreenSize();
    lua_pushinteger(L, size.Width);
    lua_pushinteger(L, size.Height);
    return 2;
}

int harvest_drawText(lua_State* L)
{
    if (gp_luaManager && lua_gettop(L) >= 3)
    {
        const char* text = lua_tostring(L, 1);
        int x = lua_tointeger(L, 2);
        int y = lua_tointeger(L, 3);
        int alignment = 0;
        if (lua_gettop(L) >= 4)
            alignment = lua_tointeger(L, 4);
        ox::video::SColor color(0xffffffff);
        if (lua_gettop(L) >= 7)
        {
            color.setRed(lua_tointeger(L, 5));
            color.setGreen(lua_tointeger(L, 6));
            color.setBlue(lua_tointeger(L, 7));
        }
        if (lua_gettop(L) >= 8)
            color.setAlpha(lua_tointeger(L, 8));
        gp_luaManager->addTextObject(ox::core::CString<wchar_t>(text), ox::core::CPosition2d<int>(x, y),
            alignment, color);
    }
    return 0;
}

int harvest_drawLine(lua_State* L)
{
    if (gp_luaManager && lua_gettop(L) >= 4)
    {
        int x1 = lua_tointeger(L, 1);
        int y1 = lua_tointeger(L, 2);
        int x2 = lua_tointeger(L, 3);
        int y2 = lua_tointeger(L, 4);
        ox::video::SColor color(0xffffffff);
        if (lua_gettop(L) >= 7)
        {
            color.setRed(lua_tointeger(L, 5));
            color.setGreen(lua_tointeger(L, 6));
            color.setBlue(lua_tointeger(L, 7));
        }
        if (lua_gettop(L) >= 8)
            color.setAlpha(lua_tointeger(L, 8));
        gp_luaManager->addLineObject(ox::core::CPosition2d<int>(x1, y1), ox::core::CPosition2d<int>(x2, y2),
            color);
    }
    return 0;
}

int harvest_drawRectangle(lua_State* L)
{
    if (gp_luaManager && lua_gettop(L) >= 4)
    {
        int x = lua_tointeger(L, 1);
        int y = lua_tointeger(L, 2);
        int width = lua_tointeger(L, 3);
        int height = lua_tointeger(L, 4);
        ox::video::SColor color(0xffffffff);
        if (lua_gettop(L) >= 7)
        {
            color.setRed(lua_tointeger(L, 5));
            color.setGreen(lua_tointeger(L, 6));
            color.setBlue(lua_tointeger(L, 7));
        }
        if (lua_gettop(L) >= 8)
            color.setAlpha(lua_tointeger(L, 8));
        gp_luaManager->addRectangleObject(ox::core::CPosition2d<int>(x, y),
            ox::core::CDimension2d<int>(width, height), color);
    }
    return 0;
}

static const luaL_Reg g_harvestSystemLib[] =
{
    { "print", harvest_print },
    { "setThreatLevelProgress", harvest_setThreatLevelProgress },
    { "setThreatLevelValue", harvest_setThreatLevelValue },
    { "getCredits", harvest_getCredits },
    { "addCredits", harvest_addCredits },
    { "getPlanet", harvest_getPlanet },
    { "getWorldBorders", harvest_getWorldBorders },
    { "isAlienAvailable", harvest_isAlienAvailable },
    { "isPositionBlocked", harvest_isPositionBlocked },
    { "spawnAlien", harvest_spawnAlien },
    { "spawnBuilding", harvest_spawnBuilding },
    { "spawnChargeBomb", harvest_spawnChargeBomb },
    { "spawnMissile", harvest_spawnMissile },
    { "spawnEagleMissile", harvest_spawnEagleMissile },
    { "spawnTempestMissile", harvest_spawnTempestMissile },
    { "spawnParticle", harvest_spawnParticle },
    { "removeMinerals", harvest_removeMinerals },
    { "spawnMinerals", harvest_spawnMinerals },
    { "screenToWorldCoordinates", harvest_screenToWorldCoordinates },
    { "worldToScreenCoordinates", harvest_worldToScreenCoordinates },
    { "getScreenSize", harvest_getScreenSize },
    { "drawText", harvest_drawText },
    { "drawLine", harvest_drawLine },
    { "drawRectangle", harvest_drawRectangle },
    { "findBuildings", harvest_findBuildings },
    { "findAliens", harvest_findAliens },
    { "getBuilding", harvest_getBuilding },
    { "getAlien", harvest_getAlien },
    { "setCreativeListVisible", harvest_setCreativeListVisible },
    { "setRushListVisible", harvest_setRushListVisible },
    { "setRushProgress", harvest_setRushProgress },
    { "setWaveListVisible", harvest_setWaveListVisible },
    { "setTimerVisible", harvest_setTimerVisible },
    { "setTimerValue", harvest_setTimerValue },
    { "setWaveButtonVisible", harvest_setWaveButtonVisible },
    { "setWaveButtonAliens", harvest_setWaveButtonAliens },
    { "setWaveButtonLabel", harvest_setWaveButtonLabel },
    { "showInfoMessage", harvest_showInfoMessage },
    { "getNumAliens", harvest_getNumAliens },
    { "getNumBuildings", harvest_getNumBuildings },
    { "winGame", harvest_winGame },
    { "loseGame", harvest_loseGame },
    { "setMinimumWorldBorders", harvest_setMinimumWorldBorders },
    { "getTotalAlienDamage", harvest_getTotalAlienDamage },
    { "dofile", harvest_dofile },
    { "addSpriteState", harvest_addSpriteState },
    { "removeSpriteState", harvest_removeSpriteState },
    { "renderSpriteState", harvest_renderSpriteState },
    { "getViewPosition", harvest_getViewPosition },
    { "setViewPosition", harvest_setViewPosition },
    { "isKeyPressed", harvest_isKeyPressed },
    { "getMousePosition", harvest_getMousePosition },
    { "setBuildingEnabled", harvest_setBuildingEnabled },
    { "defineUpgrade", harvest_defineUpgrade },
    { "defineActionButton", harvest_defineActionButton },
    { "defineUpgradeButton", harvest_defineUpgradeButton },
    { "spawnEnergySpark", harvest_spawnEnergySpark },
    { "spawnBullet", harvest_spawnBullet },
    { "getSelectedBuilding", harvest_getSelectedBuilding },
    { "getSelectedBuildings", harvest_getSelectedBuildings },
    { "renderSpriteStateFreeShape", harvest_renderSpriteStateFreeShape },
    { 0, 0 }
};

CLuaManager::CLuaManager(ox::IOxDevice* device, ox::event::IEventReceiver* receiver)
    : Device(device), EventReceiver(receiver), L(0), HookFunction(-1), ThreatLevelProgress(0),
      ThreatLevelValue(0), CreativeListVisible(true), WaveListVisible(false), RushListVisible(false),
      TimerVisible(false), RushProgress(0), TimerValue(0), SelectedBuilding(0), SelectedBuildings(0),
      FreeSpriteState(0)
{
    gp_luaManager = this;
    for (int i = 0; i < 22; ++i)
        HookStates[i] = 0;
    for (int i = 0; i < 10; ++i)
    {
        WaveButtonVisible[i] = true;
        WaveButtonNumber[i] = 1;
        for (int j = 0; j < 14; ++j)
            WaveButtonAliens[i][j] = false;
    }
}

CLuaManager::~CLuaManager()
{
    if (L)
    {
        lua_close(L);
        gp_luaState = 0;
    }
    gp_luaManager = 0;

    for (unsigned int i = 0; i < GuiObjects.size(); ++i)
        delete GuiObjects[i];
    for (unsigned int i = 0; i < SpriteStates.size(); ++i)
        if (SpriteStates[i])
            SpriteStates[i]->remove();
}

void CLuaManager::initCommonLuaStuff()
{
    RunningMods.clear();
    L = luaL_newstate();
    luaL_openlibs(L);
    luaL_register(L, "harvest", g_harvestSystemLib);
    Lunar<entity::CCreativeLuaState>::Register(L);
    Lunar<entity::CBuildingLuaInfo>::Register(L);
    Lunar<entity::CAlienLuaInfo>::Register(L);

    ox::io::IFilePath* path =
        Device->getFileSystem()->resolveAliases("$GAME_RESOURCES$/harvestClientData/mods/harvest.lua");
    ox::core::CString<char> filename = path->getPath();
    path->drop();

    if (luaL_loadfile(L, filename.c_str()) != 0 || lua_pcall(L, 0, LUA_MULTRET, 0) != 0)
        CompilerErrors.push_back(ox::core::CString<char>(lua_tostring(L, -1)));

    lua_getfield(L, LUA_GLOBALSINDEX, "hook");
    lua_getfield(L, -1, "call");
    HookFunction = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pop(L, 1);
    gp_luaState = L;
}

void CLuaManager::includeMod(const SLuaMod& mod)
{
    ox::core::CString<char> filename = mod.Path;
    filename.append(ox::core::CString<char>("main.lua"));
    ox::io::IReadFile* file = Device->getFileSystem()->createAndOpenFile(filename.c_str());
    if (file)
    {
        ox::core::CString<char> script = "-- FILE@";
        script.append(filename);
        script.append(ox::core::CString<char>("\n"));
        ox::core::CString<char> content;
        ox::io::CHelpIO::readString(file, content);
        script.append(content);
        if (luaL_loadstring(L, script.c_str()) == 0 && lua_pcall(L, 0, LUA_MULTRET, 0) == 0)
            RunningMods.push_back(mod);
        else
            CompilerErrors.push_back(ox::core::CString<char>(lua_tostring(L, -1)));
        file->drop();
    }
}

void CLuaManager::initLuaByScriptList()
{
    initCommonLuaStuff();
    for (unsigned int i = 0; i < s_availableLuaScriptFiles.size(); ++i)
        if (s_availableLuaScriptFiles[i].Enabled)
            includeMod(s_availableLuaScriptFiles[i]);
}

bool CLuaManager::initLuaBySaveFile(ox::io::IReadFile* file, int version)
{
    refreshAvailableLuaMods(Device);
    int numMods = ox::io::CHelpIO::readInt(file);
    if (numMods <= 0)
        return false;

    initCommonLuaStuff();
    for (int i = 0; i < numMods; ++i)
    {
        ox::core::CString<char> folder;
        ox::io::CHelpIO::readString(file, folder);
        for (unsigned int j = 0; j < s_availableLuaScriptFiles.size(); ++j)
        {
            if (folder == s_availableLuaScriptFiles[j].Folder)
            {
                s_availableLuaScriptFiles[j].Enabled = true;
                includeMod(s_availableLuaScriptFiles[j]);
                break;
            }
        }
    }

    int numValues = ox::io::CHelpIO::readInt(file);
    ox::TArray<SLuaFilePair> values;
    for (int i = 0; i < numValues; ++i)
    {
        SLuaFilePair pair;
        CLuaFileValues::readLuaFilePair(file, pair, version);
        values.push_back(pair);
    }

    lua_newtable(L);
    int table = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_rawgeti(L, LUA_REGISTRYINDEX, table);
    CLuaFileValues::getLuaTableVars(L, table, ox::core::CString<char>(""), values);
    lua_pop(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, HookFunction);
    lua_pushstring(L, "gameLoad");
    lua_rawgeti(L, LUA_REGISTRYINDEX, table);
    lua_pcall(L, 2, 1, 0);
    lua_pop(L, 1);
    luaL_unref(L, LUA_REGISTRYINDEX, table);
    return true;
}

void CLuaManager::refreshAvailableLuaMods(ox::IOxDevice* device)
{
    s_availableLuaScriptFiles.clear();
    ox::io::IFileSystem* fileSystem = device->getFileSystem();

    ox::core::CString<char> directory = "$GAME_RESOURCES$/harvestClientData/mods/";
    ox::io::IFileList* files = fileSystem->createFileList("*.hmd", directory.c_str(), (ox::io::EFileList)1);
    addModsFromFiles(files, device, directory);
    files->drop();

    ox::io::IFileList* archives = fileSystem->createFileList("*.zip", directory.c_str(), (ox::io::EFileList)1);
    for (int i = 0; i < archives->getFileCount(); ++i)
    {
        ox::core::CString<char> archive = directory;
        archive.append(ox::core::CString<char>(archives->getFileName(i)));
        ox::io::IFileList* list = fileSystem->createFileList("*.hmd", archive.c_str(), (ox::io::EFileList)0);
        if (list)
            addModsFromFiles(list, device, archive);
        list->drop();
    }
    archives->drop();

    ox::core::CString<char> userDirectory = "$HARVEST_USERDATA$/mods/";
    files = fileSystem->createFileList("*.hmd", userDirectory.c_str(), (ox::io::EFileList)1);
    addModsFromFiles(files, device, userDirectory);
    files->drop();

    archives = fileSystem->createFileList("*.zip", userDirectory.c_str(), (ox::io::EFileList)1);
    for (int i = 0; i < archives->getFileCount(); ++i)
    {
        ox::core::CString<char> archive = userDirectory;
        archive.append(ox::core::CString<char>(archives->getFileName(i)));
        ox::io::IFileList* list = fileSystem->createFileList("*.hmd", archive.c_str(), (ox::io::EFileList)0);
        if (list)
            addModsFromFiles(list, device, archive);
        list->drop();
    }
    archives->drop();
}

void CLuaFileValues::readLuaFilePair(ox::io::IReadFile* file, SLuaFilePair& pair, int version)
{
    readLuaAttribute(file, pair.Key, version);
    readLuaAttribute(file, pair.Value, version);
}

void CLuaFileValues::getLuaTableVars(lua_State* L, int table, const ox::core::CString<char>& prefix,
    ox::TArray<SLuaFilePair>& values)
{
    int start = prefix.size();
    ox::TArray<SLuaFilePair>::iterator it = values.begin();
    while (it != values.end())
    {
        if (start != 0 && !it->Key.String.startsWith(prefix))
        {
            ++it;
            continue;
        }

        int dot = 0;
        while (start + dot < it->Key.String.size() && it->Key.String[start + dot] != '.')
            ++dot;

        if (start + dot < it->Key.String.size())
        {
            ox::core::CString<char> subPrefix = it->Key.String.subString(0, start + dot + 1);
            ox::core::CString<char> key = it->Key.String.subString(start, dot);
            int number = strtol(key.c_str(), 0, 10);
            if (number != 0 || key == ox::core::CString<char>("0"))
                lua_pushnumber(L, number);
            else
                lua_pushstring(L, key.c_str());
            lua_newtable(L);
            getLuaTableVars(L, lua_gettop(L), subPrefix, values);
            lua_settable(L, table);
            it = values.begin();
        }
        else
        {
            ox::core::CString<char> key = it->Key.String.subString(start, it->Key.String.size() - start);
            lua_pushstring(L, key.c_str());
            if (it->Value.IsString)
                lua_pushstring(L, it->Value.String.c_str());
            else
                lua_pushnumber(L, it->Value.Number);
            lua_settable(L, table);
            it = values.erase(it);
        }
    }
}

bool CLuaManager::writeLuaStates(ox::io::IWriteFile* file)
{
    ox::io::CHelpIO::writeInt(file, RunningMods.size());
    for (unsigned int i = 0; i < RunningMods.size(); ++i)
        ox::io::CHelpIO::writeString(file, RunningMods[i].Folder);

    lua_newtable(L);
    int table = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_rawgeti(L, LUA_REGISTRYINDEX, HookFunction);
    lua_pushstring(L, "gameSave");
    lua_rawgeti(L, LUA_REGISTRYINDEX, table);
    lua_pcall(L, 2, 1, 0);
    lua_pop(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, table);

    ox::TArray<SLuaFilePair> values;
    CLuaFileValues::addLuaTableVars(gp_luaState, ox::core::CString<char>(""), values);
    ox::io::CHelpIO::writeInt(file, values.size());
    for (unsigned int i = 0; i < values.size(); ++i)
        CLuaFileValues::writeLuaFilePair(file, values[i]);
    luaL_unref(L, LUA_REGISTRYINDEX, table);
    return true;
}

void CLuaFileValues::addLuaTableVars(lua_State* L, const ox::core::CString<char>& prefix,
    ox::TArray<SLuaFilePair>& values)
{
    lua_pushnil(L);
    while (lua_next(L, -2))
    {
        SLuaFilePair pair;
        int keyType = lua_type(L, -2);
        ox::core::CString<char> key;
        if (keyType == LUA_TNUMBER)
        {
            pair.Key.Number = (float)lua_tonumber(L, -2);
            key = ox::core::CString<char>((int)pair.Key.Number);
        }
        else
            key = lua_tostring(L, -2);

        pair.Key.IsString = true;
        pair.Key.String = prefix;
        pair.Key.String.append(key);

        if (lua_isnumber(L, -1))
        {
            pair.Value.IsString = false;
            pair.Value.Number = (float)lua_tonumber(L, -1);
            values.push_back(pair);
        }
        else if (lua_isstring(L, -1))
        {
            pair.Value.String = lua_tostring(L, -1);
            pair.Value.IsString = true;
            values.push_back(pair);
        }
        else if (lua_type(L, -1) == LUA_TTABLE)
        {
            ox::core::CString<char> subPrefix = prefix;
            subPrefix.append(key);
            subPrefix.append(ox::core::CString<char>("."));
            addLuaTableVars(L, subPrefix, values);
        }
        lua_pop(L, 1);
    }
}

void CLuaFileValues::writeLuaFilePair(ox::io::IWriteFile* file, SLuaFilePair& pair)
{
    writeLuaAttribute(file, pair.Key);
    writeLuaAttribute(file, pair.Value);
}

bool CLuaManager::pushHooker(const char* name, int hook)
{
    if (HookFunction < 0 || !L || HookStates[hook] == 2)
        return false;
    lua_rawgeti(L, LUA_REGISTRYINDEX, HookFunction);
    lua_pushstring(L, name);
    return true;
}

void CLuaManager::checkHooker(int hook)
{
    if (HookStates[hook] == 0)
    {
        int result = lua_tointeger(L, -1);
        if (result)
            HookStates[hook] = 2;
        else
            HookStates[hook] = 1;
    }
    lua_pop(L, 1);
}

ox::TArray<ox::core::CString<char> >& CLuaManager::getCompilerErrors()
{
    return CompilerErrors;
}

int CLuaManager::getNumAvailableLuaMods()
{
    return RunningMods.size();
}

ox::TArray<SLuaMod>& CLuaManager::getAvailableLuaMods()
{
    return RunningMods;
}

void CLuaManager::hookNewGame()
{
    if (!pushHooker("gameInit", 0))
        return;
    if (lua_pcall(L, 1, 1, 0) == 0)
        checkHooker(0);
    else
        lua_tostring(L, -1);
}

void CLuaManager::runFrameFunctions(float frameDelta)
{
    updateAllSpriteStates(frameDelta);
    if (!pushHooker("frameUpdate", 3))
        return;
    lua_pushnumber(L, frameDelta);
    if (lua_pcall(L, 2, 1, 0) == 0)
    {
        checkHooker(3);
        return;
    }
    ox::core::CString<char> error = lua_tostring(L, -1);
    AlwaysAppendToFile("luaoutput.txt", "lua error in frameUpdate: %s\n", error.c_str());
    postStringAsInfo(error);
}

void CLuaManager::updateAllSpriteStates(float frameDelta)
{
    for (unsigned int i = 0; i < RemovedSpriteStates.size(); ++i)
        removeSpriteState(RemovedSpriteStates[i]);
    RemovedSpriteStates.clear();

    for (unsigned int i = 0; i < SpriteStates.size(); ++i)
        if (SpriteStates[i])
            SpriteStates[i]->update(frameDelta);
}

void CLuaManager::postStringAsInfo(const ox::core::CString<char>& text)
{
    ox::core::CString<wchar_t> message = text.c_str();
    ox::event::SEvent event;
    event.EventType = ox::event::EET_USER_EVENT;
    event.UserEvent.UserData1 = 33;
    event.UserEvent.UserData2 = 0;
    event.UserEvent.UserData3 = 0;
    event.UserEvent.UserPointer = (void*)message.c_str();
    if (gp_luaManager)
        gp_luaManager->EventReceiver->OnEvent(event);
    else
        ox::event::gp_subscriberList->OnEvent(event);
}

void CLuaManager::hookMouseClick(const ox::core::CPosition2d<float>& position, int button, bool pressed)
{
    if (!pushHooker("mouseButton", 4))
        return;
    lua_pushnumber(L, position.X);
    lua_pushnumber(L, position.Y);
    lua_pushboolean(L, pressed);
    lua_pushinteger(L, button);
    if (lua_pcall(L, 5, 1, 0) == 0)
        checkHooker(4);
    else
        lua_tostring(L, -1);
}

void CLuaManager::hookAlienDeath(int alienType, const ox::core::CVector3d<float>& position)
{
    if (!pushHooker("alienDeath", 5))
        return;
    lua_pushinteger(L, alienType + 1);
    lua_pushnumber(L, position.X);
    lua_pushnumber(L, position.Y);
    lua_pushnumber(L, position.Z);
    if (lua_pcall(L, 5, 1, 0) == 0)
        checkHooker(5);
    else
        lua_tostring(L, -1);
}

void CLuaManager::hookAlienSpawned(entity::CAlienEntity* alien)
{
    if (alien && alien->getLuaInfo() && pushHooker("alienPlaced", 6))
    {
        Lunar<entity::CAlienLuaInfo>::push(L, alien->getLuaInfo());
        if (lua_pcall(L, 2, 1, 0) == 0)
            checkHooker(6);
    }
}

void CLuaManager::hookBuildingPlaced(entity::CBuildingEntity* building, const char* buildingType)
{
    if (pushHooker("buildingPlaced", 7) && building && building->getLuaInfo())
    {
        Lunar<entity::CBuildingLuaInfo>::push(L, building->getLuaInfo());
        lua_pushstring(L, buildingType);
        if (lua_pcall(L, 3, 1, 0) == 0)
            checkHooker(7);
    }
}

void CLuaManager::hookBuildingConstructed(entity::CBuildingEntity* building)
{
    if (pushHooker("buildingCompleted", 8) && building && building->getLuaInfo())
    {
        Lunar<entity::CBuildingLuaInfo>::push(L, building->getLuaInfo());
        if (lua_pcall(L, 2, 1, 0) == 0)
            checkHooker(8);
    }
}

void CLuaManager::hookBuildingDestroyed(const char* buildingType, float x, float y, entity::CAlienEntity* alien)
{
    if (alien && alien->getLuaInfo() && pushHooker("buildingDestroyed", 9))
    {
        lua_pushstring(L, buildingType);
        lua_pushnumber(L, x);
        lua_pushnumber(L, y);
        Lunar<entity::CAlienLuaInfo>::push(L, alien->getLuaInfo());
        if (lua_pcall(L, 5, 1, 0) == 0)
            checkHooker(9);
    }
}

void CLuaManager::hookBuildingSold(const char* buildingType, float x, float y, int credits)
{
    if (pushHooker("buildingSold", 12))
    {
        lua_pushstring(L, buildingType);
        lua_pushnumber(L, x);
        lua_pushnumber(L, y);
        lua_pushinteger(L, credits);
        if (lua_pcall(L, 5, 1, 0) == 0)
            checkHooker(12);
    }
}

void CLuaManager::hookUnitSelected(int id)
{
    if (pushHooker("unitSelected", 10))
    {
        lua_pushinteger(L, id);
        if (lua_pcall(L, 2, 1, 0) == 0)
            checkHooker(10);
    }
}

void CLuaManager::hookCreditsMined(entity::CBuildingEntity* miner, int mineralsId)
{
    if (pushHooker("creditsMined", 11) && miner && miner->getLuaInfo())
    {
        Lunar<entity::CBuildingLuaInfo>::push(L, miner->getLuaInfo());
        lua_pushinteger(L, mineralsId);
        if (lua_pcall(L, 3, 1, 0) == 0)
            checkHooker(11);
    }
}

void CLuaManager::hookMinerOutOfMinerals(entity::CBuildingEntity* miner)
{
    if (pushHooker("minerOutOfMinerals", 13) && miner && miner->getLuaInfo())
    {
        Lunar<entity::CBuildingLuaInfo>::push(L, miner->getLuaInfo());
        if (lua_pcall(L, 2, 1, 0) == 0)
            checkHooker(13);
    }
}

void CLuaManager::hookEnergySparkCreated(int sparkId, entity::CBuildingEntity* building)
{
    if (pushHooker("sparkCreated", 14) && building && building->getLuaInfo())
    {
        lua_pushinteger(L, sparkId);
        Lunar<entity::CBuildingLuaInfo>::push(L, building->getLuaInfo());
        if (lua_pcall(L, 3, 1, 0) == 0)
            checkHooker(14);
    }
}

void CLuaManager::hookMissileLaunched(int missileType, entity::CBuildingEntity* turret, int targetId,
    float x, float y)
{
    if (pushHooker("missileLaunched", 15) && turret && turret->getLuaInfo())
    {
        lua_pushinteger(L, missileType);
        Lunar<entity::CBuildingLuaInfo>::push(L, turret->getLuaInfo());
        if (targetId > 0)
        {
            lua_pushinteger(L, targetId);
            lua_pushnil(L);
            lua_pushnil(L);
        }
        else
        {
            lua_pushnil(L);
            lua_pushnumber(L, x);
            lua_pushnumber(L, y);
        }
        if (lua_pcall(L, 6, 1, 0) == 0)
            checkHooker(15);
    }
}

void CLuaManager::hookEnergyLinkOverheated(entity::CBuildingEntity* link)
{
    if (pushHooker("energyLinkOverheated", 16) && link && link->getLuaInfo())
    {
        Lunar<entity::CBuildingLuaInfo>::push(L, link->getLuaInfo());
        if (lua_pcall(L, 2, 1, 0) == 0)
            checkHooker(16);
    }
}

void CLuaManager::hookEnergyLinkCharging(entity::CBuildingEntity* link)
{
    if (pushHooker("energyLinkCharging", 17) && link && link->getLuaInfo())
    {
        Lunar<entity::CBuildingLuaInfo>::push(L, link->getLuaInfo());
        if (lua_pcall(L, 2, 1, 0) == 0)
            checkHooker(17);
    }
}

void CLuaManager::hookEnergyLinkCharged(float x, float y)
{
    if (pushHooker("energyLinkCharged", 18))
    {
        lua_pushnumber(L, x);
        lua_pushnumber(L, y);
        if (lua_pcall(L, 3, 1, 0) == 0)
            checkHooker(18);
    }
}

void CLuaManager::hookMapExpanded(float left, float top, float right, float bottom)
{
    if (pushHooker("mapExpanded", 19))
    {
        lua_pushnumber(L, left);
        lua_pushnumber(L, top);
        lua_pushnumber(L, right);
        lua_pushnumber(L, bottom);
        if (lua_pcall(L, 5, 1, 0) == 0)
            checkHooker(19);
    }
}

void CLuaManager::hookWaveButton(int button)
{
    if (pushHooker("waveButton", 20))
    {
        lua_pushinteger(L, button + 1);
        if (lua_pcall(L, 2, 1, 0) == 0)
            checkHooker(20);
    }
}

void CLuaManager::hookTextInput(const wchar_t* text)
{
    ox::core::CString<char> input = text;
    if (input.size() > 0 && pushHooker("textInput", 21) == true)
    {
        lua_pushstring(L, input.c_str());
        if (lua_pcall(L, 2, 1, 0) == 0)
            checkHooker(21);
    }
}

void CLuaManager::hookCreativeInit(const ox::core::CString<char>& id, entity::CCreativeEntity* building)
{
    ox::core::CString<char> name = id.c_str();
    name.append(ox::core::CString<char>("_Init"));
    if (building && building->getCreativeLuaState() && pushCreativeHooker(name.c_str(), 1) == true)
    {
        Lunar<entity::CCreativeLuaState>::push(L, building->getCreativeLuaState());
        building->getCreativeLuaState()->getPrivates(L);
        if (lua_pcall(L, 3, 1, 0) == 0)
            checkCreativeHooker(name.c_str(), 1);
    }
}

bool CLuaManager::pushCreativeHooker(const char* name, int hookSet)
{
    if (HookFunction < 0 || !L)
        return false;

    ox::TArray<SCreativeHook>& hooks = CreativeHooks[hookSet];
    unsigned int i;
    for (i = 0; i < hooks.size(); ++i)
    {
        if (hooks[i].Name == ox::core::CString<char>(name))
        {
            if (hooks[i].State == 2)
                return false;
            break;
        }
    }
    if (i >= hooks.size())
    {
        SCreativeHook hook;
        hook.Name = name;
        hook.State = 0;
        hooks.push_back(hook);
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, HookFunction);
    lua_pushstring(L, name);
    return true;
}

void CLuaManager::checkCreativeHooker(const char* name, int hookSet)
{
    int result = lua_tointeger(L, -1);
    ox::TArray<SCreativeHook>& hooks = CreativeHooks[hookSet];
    for (unsigned int i = 0; i < hooks.size(); ++i)
    {
        if (hooks[i].Name == ox::core::CString<char>(name))
        {
            hooks[i].State = result ? 2 : 1;
            return;
        }
    }
}

void CLuaManager::hookCreativeUpdate(const ox::core::CString<char>& id, entity::CCreativeEntity* building,
    float frameDelta)
{
    ox::core::CString<char> name = id.c_str();
    name.append(ox::core::CString<char>("_Update"));
    if (building && building->getCreativeLuaState() && pushCreativeHooker(name.c_str(), 1) == true)
    {
        Lunar<entity::CCreativeLuaState>::push(L, building->getCreativeLuaState());
        building->getCreativeLuaState()->getPrivates(L);
        lua_pushnumber(L, frameDelta);
        if (lua_pcall(L, 4, 1, 0) == 0)
            checkCreativeHooker(name.c_str(), 1);
    }
}

bool CLuaManager::isRunningMods()
{
    return !RunningMods.empty();
}

float CLuaManager::getThreatLevelProgress()
{
    return ThreatLevelProgress;
}

int CLuaManager::getThreatLevelValue()
{
    return ThreatLevelValue;
}

void CLuaManager::setThreatLevelProgress(float progress)
{
    ThreatLevelProgress = progress;
}

void CLuaManager::setThreatLevelValue(int value)
{
    ThreatLevelValue = value;
}

void CLuaManager::updateSelectedBuilding(entity::CEntity* building)
{
    if (building && building->getEntityType() != 5)
        SelectedBuilding = building;
    else
        SelectedBuilding = 0;
}

void CLuaManager::updateSelectedBuildings(ox::TArray<ox::entity::SEntityReference*>* buildings)
{
    SelectedBuildings = buildings;
}

ox::core::CPosition2d<int> CLuaManager::worldToScreen(const ox::core::CPosition2d<float>& position)
{
    return ox::core::CPosition2d<int>((int)(position.X - ViewPosition.X), (int)(position.Y - ViewPosition.Y));
}

ox::core::CPosition2d<float> CLuaManager::screenToWorld(const ox::core::CPosition2d<int>& position)
{
    return ox::core::CPosition2d<float>(position.X + ViewPosition.X, position.Y + ViewPosition.Y);
}

void CLuaManager::addTextObject(const ox::core::CString<wchar_t>& text, const ox::core::CPosition2d<int>& position,
    int alignment, ox::video::SColor color)
{
    SLuaGuiObject* object = new SLuaGuiObject;
    object->Type = 0;
    object->Text = text;
    object->X = position.X;
    object->Y = position.Y;
    object->Extent1 = alignment;
    object->Color = color;
    GuiObjects.push_back(object);
}

void CLuaManager::addLineObject(const ox::core::CPosition2d<int>& start, const ox::core::CPosition2d<int>& end,
    ox::video::SColor color)
{
    SLuaGuiObject* object = new SLuaGuiObject;
    object->Type = 1;
    object->X = start.X;
    object->Y = start.Y;
    object->Extent1 = end.X;
    object->Extent2 = end.Y;
    object->Color = color;
    GuiObjects.push_back(object);
}

void CLuaManager::addRectangleObject(const ox::core::CPosition2d<int>& position,
    const ox::core::CDimension2d<int>& size, ox::video::SColor color)
{
    SLuaGuiObject* object = new SLuaGuiObject;
    object->Type = 2;
    object->X = position.X;
    object->Y = position.Y;
    object->Extent1 = size.Width;
    object->Extent2 = size.Height;
    object->Color = color;
    GuiObjects.push_back(object);
}

void CLuaManager::renderGuiObjects(ox::video::IVideoDriver* driver, ox::gui::IGUIFont* font)
{
    for (unsigned int i = 0; i < GuiObjects.size(); ++i)
    {
        SLuaGuiObject* object = GuiObjects[i];
        switch (object->Type)
        {
        case 2:
            driver->draw2DRectangle(object->Color, ox::core::CRect<int>(object->X, object->Y,
                object->X + object->Extent1, object->Y + object->Extent2), 0);
            break;
        case 1:
            driver->draw2DLine(ox::core::CPosition2d<int>(object->X, object->Y),
                ox::core::CPosition2d<int>(object->Extent1, object->Extent2), object->Color);
            break;
        case 0:
        {
            int x = object->X;
            int y = object->Y;
            ox::core::CDimension2d<int> size = font->getDimension(object->Text.c_str());
            if (GuiObjects[i]->Extent1 == 1)
                x -= size.Width / 2;
            else if (GuiObjects[i]->Extent1 == 2)
                x -= size.Width;
            font->draw(GuiObjects[i]->Text.c_str(), ox::core::CRect<int>(x, y, x + size.Width, y + size.Height),
                GuiObjects[i]->Color, (ox::gui::EFontHorizontalAlign)0, (ox::gui::EFontVerticalAlign)0, 0);
            break;
        }
        }
        delete GuiObjects[i];
    }
    GuiObjects.clear();
}

bool CLuaManager::isWaveButtonVisible(int button)
{
    return WaveButtonVisible[button];
}

bool CLuaManager::isAlienOnButton(int button, int alien)
{
    return WaveButtonAliens[button][alien];
}

int CLuaManager::getWaveButtonNumber(int button)
{
    return WaveButtonNumber[button];
}

bool CLuaManager::isCreativeListVisible()
{
    return CreativeListVisible;
}

bool CLuaManager::isRushListVisible()
{
    return RushListVisible;
}

bool CLuaManager::isWaveListVisible()
{
    return WaveListVisible;
}

bool CLuaManager::isTimerVisible()
{
    return TimerVisible;
}

float CLuaManager::getRushProgress()
{
    return RushProgress;
}

float CLuaManager::getTimerValue()
{
    return TimerValue;
}

const ox::core::CRect<float>& CLuaManager::getMinimumWorldBorders()
{
    return MinimumWorldBorders;
}

void CLuaManager::removeSpriteState(int index)
{
    if (index < 0 || index >= (int)SpriteStates.size() || !SpriteStates[index])
        return;
    SpriteStates[index]->remove();
    SpriteStates[index] = 0;
    if (FreeSpriteState > index)
        FreeSpriteState = index;
}

SLuaEntityActionButton* CLuaManager::getEntityActionButton(int id, const char* entityId)
{
    for (unsigned int i = 0; i < EntityActionButtons.size(); ++i)
        if (EntityActionButtons[i].Id == id &&
            EntityActionButtons[i].EntityId == ox::core::CString<char>(entityId))
            return &EntityActionButtons[i];
    return 0;
}

SLuaEntityActionButton* CLuaManager::getNextEntityActionButton(const char* entityId,
    SLuaEntityActionButton* previous)
{
    unsigned int i = 0;
    if (previous)
    {
        for (; i < EntityActionButtons.size(); ++i)
        {
            if (&EntityActionButtons[i] == previous)
            {
                ++i;
                break;
            }
        }
    }
    for (; i < EntityActionButtons.size(); ++i)
        if (EntityActionButtons[i].EntityId == ox::core::CString<char>(entityId))
            return &EntityActionButtons[i];
    return 0;
}

void CLuaManager::addModsFromFiles(ox::io::IFileList* files, ox::IOxDevice* device,
    const ox::core::CString<char>& directory)
{
    ox::io::IFileSystem* fileSystem = device->getFileSystem();
    ox::core::CString<char> path = directory;
    path.replace('\\', '/');
    if (!path.endsWith(ox::core::CString<char>("/")))
        path.append(ox::core::CString<char>("/"));

    for (int i = 0; i < files->getFileCount(); ++i)
    {
        ox::game::CConfiguration* config = new ox::game::CConfiguration(fileSystem);
        ox::core::CString<char> filename = path;
        filename.append(ox::core::CString<char>(files->getFileName(i)));
        if (config->read(fileSystem->createAndOpenFile(filename.c_str())) == true &&
            config->attributeExists(L"mod:name") == true && config->attributeExists(L"mod:folder") == true)
        {
            SLuaMod mod;
            ox::core::CString<wchar_t> folder;
            config->getAttribute(L"mod:name", mod.Name);
            config->getAttribute(L"mod:author", mod.Author);
            config->getAttribute(L"mod:folder", folder);
            mod.Path = path;
            mod.Path.append(ox::core::CString<char>(folder.c_str()));
            mod.Path.append(ox::core::CString<char>("/"));
            mod.Folder = ox::core::CString<char>(folder.c_str());
            mod.Enabled = false;
            mod.HasIcon = false;
            ox::core::CString<char> mainFile = mod.Path;
            mainFile.append(ox::core::CString<char>("main.lua"));
            if (fileSystem->existFile(mainFile.c_str(), true) == true)
            {
                ox::core::CString<char> icon = mod.Path;
                icon.append(ox::core::CString<char>("favicon.tga"));
                if (fileSystem->existFile(icon.c_str(), true) == true)
                    mod.HasIcon = true;
                s_availableLuaScriptFiles.push_back(mod);
            }
        }
        if (config)
            delete config;
    }
}

void CLuaFileValues::readLuaAttribute(ox::io::IReadFile* file, SLuaAttribute& attribute, int version)
{
    attribute.IsString = ox::io::CHelpIO::readByte(file) != 0;
    if (attribute.IsString)
        ox::io::CHelpIO::readString(file, attribute.String);
    else
        attribute.Number = ox::io::CHelpIO::readFloat(file);
}

void CLuaFileValues::writeLuaAttribute(ox::io::IWriteFile* file, SLuaAttribute& attribute)
{
    ox::io::CHelpIO::writeByte(file, attribute.IsString);
    if (attribute.IsString)
        ox::io::CHelpIO::writeString(file, attribute.String);
    else
        ox::io::CHelpIO::writeFloat(file, attribute.Number);
}

} // end namespace game
} // end namespace harvest
