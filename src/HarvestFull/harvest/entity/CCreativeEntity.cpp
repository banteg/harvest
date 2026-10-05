// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CCreativeEntity.h"
#include "harvest/entity/CBuildableItems.h"
#include "harvest/entity/CEntityManager.h"
#include "harvest/game/CLuaManager.h"
#include "harvest/game/CLuaFileValues.h"
#include "ox/io/CHelpIO.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/ISpritePackage.h"

namespace harvest {
namespace entity {

const char CCreativeLuaState::className[] = "CCreativeLuaState";
Lunar<CCreativeLuaState>::RegType CCreativeLuaState::methods[] =
{
    { "getId", &CCreativeLuaState::getId },
    { "setRenderColorF", &CCreativeLuaState::setRenderColorF },
    { "setProgress", &CCreativeLuaState::setProgress },
    { "getPosition", &CCreativeLuaState::getPosition },
    { "setWantsEnergy", &CCreativeLuaState::setWantsEnergy },
    { "getEnergyLevel", &CCreativeLuaState::getEnergyLevel },
    { "setEnergyLevel", &CCreativeLuaState::setEnergyLevel },
    { "clearEnergyLevel", &CCreativeLuaState::clearEnergyLevel },
    { "getDeathParticle", &CCreativeLuaState::getDeathParticle },
    { "remove", &CCreativeLuaState::remove },
    { "setSprite", &CCreativeLuaState::setSprite },
    { 0, 0 }
};

int CCreativeLuaState::getId(lua_State* L)
{
    lua_pushinteger(L, Entity->getId());
    return 1;
}

int CCreativeLuaState::setRenderColorF(lua_State* L)
{
    int count = lua_gettop(L);
    if (count == 4)
    {
        double red = lua_tonumber(L, 1);
        double green = lua_tonumber(L, 2);
        double blue = lua_tonumber(L, 3);
        double alpha = lua_tonumber(L, 4);
        Entity->Color = ox::video::SColor((int)((float)alpha * 255.0f),
            (int)((float)red * 255.0f), (int)((float)green * 255.0f), (int)((float)blue * 255.0f));
    }
    else if (count == 3)
    {
        double red = lua_tonumber(L, 1);
        double green = lua_tonumber(L, 2);
        double blue = lua_tonumber(L, 3);
        Entity->Color = ox::video::SColor(Entity->Color.getAlpha(),
            (int)((float)red * 255.0f), (int)((float)green * 255.0f), (int)((float)blue * 255.0f));
    }
    return 0;
}

int CCreativeLuaState::setProgress(lua_State* L)
{
    if (lua_gettop(L) == 1) Entity->Progress = (float)lua_tonumber(L, 1);
    return 0;
}

int CCreativeLuaState::getPosition(lua_State* L)
{
    lua_pushnumber(L, Entity->getPosition().X);
    lua_pushnumber(L, Entity->getPosition().Y);
    return 2;
}

int CCreativeLuaState::setWantsEnergy(lua_State* L)
{
    if (lua_gettop(L) == 1) Entity->WantsEnergy = lua_toboolean(L, 1) != 0;
    return 0;
}

int CCreativeLuaState::getEnergyLevel(lua_State* L)
{
    lua_pushnumber(L, Entity->Energy);
    return 1;
}

int CCreativeLuaState::setEnergyLevel(lua_State* L)
{
    if (lua_gettop(L) == 1) Entity->Energy = (float)lua_tonumber(L, 1);
    return 0;
}

int CCreativeLuaState::clearEnergyLevel(lua_State* L)
{
    Entity->Energy = 0;
    return 0;
}

int CCreativeLuaState::getDeathParticle(lua_State* L)
{
    lua_pushstring(L, "BuildingExplosionSmall");
    return 1;
}

int CCreativeLuaState::remove(lua_State* L)
{
    Entity->killEntity();
    return 0;
}

int CCreativeLuaState::setSprite(lua_State* L)
{
    if (lua_gettop(L) > 0)
    {
        ox::video::ISpritePackage* package = CEntity::gp_spritePackage;
        if (lua_gettop(L) >= 2)
        {
            ox::core::CString<char> path = game::extractLuaPath(L);
            path += ox::core::CString<char>(lua_tostring(L, 2));
            package = CEntity::gp_videoDriver->getSpritePackage(path.c_str(), false);
            if (!package) return 0;
        }
        ox::video::ISpriteAnimationState* sprite = package->addNewAnimationState(
            ox::core::CString<char>(lua_tostring(L, 1)));
        if (sprite)
        {
            Entity->Sprite->remove();
            Entity->Sprite = sprite;
        }
    }
    return 0;
}

CCreativeLuaState::CCreativeLuaState(lua_State* L)
    : Entity(0), Privates(-1)
{
    lua_newtable(game::gp_luaState);
    Privates = luaL_ref(game::gp_luaState, LUA_REGISTRYINDEX);
}

CCreativeLuaState::~CCreativeLuaState()
{
    luaL_unref(game::gp_luaState, LUA_REGISTRYINDEX, Privates);
}

void CCreativeLuaState::writeLuaPrivates(ox::io::IWriteFile* file)
{
    ox::TArray<game::SLuaFilePair> values;
    if (game::gp_luaState && Privates >= 0)
    {
        lua_rawgeti(game::gp_luaState, LUA_REGISTRYINDEX, Privates);
        game::CLuaFileValues::addLuaTableVars(game::gp_luaState, ox::core::CString<char>(""), values);
    }
    ox::io::CHelpIO::writeInt(file, values.size());
    for (unsigned int i = 0; i < values.size(); ++i)
        game::CLuaFileValues::writeLuaFilePair(file, values[i]);
}

void CCreativeLuaState::readLuaPrivates(ox::io::IReadFile* file, int version)
{
    if (version < 27) return;
    int count = ox::io::CHelpIO::readInt(file);
    ox::TArray<game::SLuaFilePair> values;
    for (int i = 0; i < count; ++i)
    {
        game::SLuaFilePair pair;
        game::CLuaFileValues::readLuaFilePair(file, pair, version);
        values.push_back(pair);
    }
    if (game::gp_luaState && Privates >= 0)
    {
        lua_rawgeti(game::gp_luaState, LUA_REGISTRYINDEX, Privates);
        int table = lua_gettop(game::gp_luaState);
        game::CLuaFileValues::getLuaTableVars(game::gp_luaState, table,
            ox::core::CString<char>(""), values);
        lua_pop(game::gp_luaState, 1);
    }
}

int CCreativeLuaState::getPrivates(lua_State* L)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, Privates);
    return 1;
}

CCreativeEntity::CCreativeEntity(float x, float y, const char* id)
    : CBuildingEntity(g_nextEntityId++, 16, x, y), Energy(0), Sprite(0), Progress(1),
      WantsEnergy(false), CreativeLuaState(0)
{
    loadBuildingData(id);
}

void CCreativeEntity::loadBuildingData(const char* id)
{
    if (gp_buildableItems && id)
    {
        BuildingId = id;
        SBuildingInfoItem* info = gp_buildableItems->getBuildingInfo(gp_buildableItems->getIndexForEntityId(id));
        if (info)
        {
            BuildingName = info->Name;
            SpritePackage = info->SpritePackage;
            SpriteName = info->BuildingSpriteName;
            CollisionSize = info->CollisionSize;
            Range = 30.0f;
            MineralCost = info->MineralCost.getValue();
            loadSprite();
            if (!CreativeLuaState && game::gp_luaManager)
            {
                CreativeLuaState = new CCreativeLuaState(0);
                CreativeLuaState->setEntity(this);
                game::gp_luaManager->hookCreativeInit(BuildingId, this);
            }
        }
    }
}

void CCreativeEntity::loadSprite()
{
    ox::video::ISpritePackage* package = gp_videoDriver->getSpritePackage(SpritePackage.c_str(), false);
    if (package) Sprite = package->addNewAnimationState(ox::core::CString<char>(SpriteName.c_str()));
}

const wchar_t* CCreativeEntity::getBuildingName() const { return BuildingName.c_str(); }

CCreativeEntity::~CCreativeEntity()
{
    if (Sprite) Sprite->remove();
    if (CreativeLuaState) delete CreativeLuaState;
}

float CCreativeEntity::getCollisionSize() const { return CollisionSize; }

int CCreativeEntity::updateLogic(float frameDelta)
{
    if (!Sprite) return 1;
    if (game::gp_luaManager) game::gp_luaManager->hookCreativeUpdate(BuildingId, this, frameDelta);
    return 0;
}

void CCreativeEntity::updateSprite(float frameDelta)
{
    if (Sprite) Sprite->update(frameDelta);
}

const char* CCreativeEntity::getBuildingId() { return BuildingId.c_str(); }

int CCreativeEntity::onSpark(CSparkEntity* spark)
{
    if (!WantsEnergy) return -1;
    Energy += 1.0f;
    return 0;
}

bool CCreativeEntity::wantsSpark() { return WantsEnergy; }

void CCreativeEntity::render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort)
{
    CEntity::renderSpriteFixed(camera, viewPort, Sprite);
    if (Progress < 1.0f && Progress >= 0)
        CEntity::renderSelfProgress(camera, viewPort, Progress, ENERGY_PROGRESS_COLOR);
}

ox::video::ISpriteAnimationState* CCreativeEntity::getCurrentDisplaySprite() { return Sprite; }
ox::core::CString<wchar_t> CCreativeEntity::getMiniStatString() { return L""; }
ox::core::CString<wchar_t> CCreativeEntity::getInfoString() { return L""; }

void CCreativeEntity::writeEntityData(ox::io::IWriteFile* file)
{
    ox::io::CHelpIO::writeFloat(file, Energy);
    ox::io::CHelpIO::writeString(file, BuildingId);
    if (CreativeLuaState) CreativeLuaState->writeLuaPrivates(file);
    ox::io::CHelpIO::writeByte(file, 255);
    ox::io::CHelpIO::writeByte(file, 247);
    ox::io::CHelpIO::writeByte(file, 255);
    ox::io::CHelpIO::writeByte(file, 247);
}

void CCreativeEntity::readEntityData(ox::io::IReadFile* file, int version)
{
    Energy = ox::io::CHelpIO::readFloat(file);
    ox::core::CString<char> id;
    ox::io::CHelpIO::readString(file, id);
    loadBuildingData(id.c_str());
    if (CreativeLuaState) CreativeLuaState->readLuaPrivates(file, version);
    if (version >= 27)
    {
        int count = 0;
        while (count < 4)
        {
            unsigned char value = ox::io::CHelpIO::readByte(file);
            if (((count & 1) && value == 247) || (!(count & 1) && value == 255)) ++count;
            else count = 0;
        }
    }
}

int CCreativeEntity::getSellValue() const { return MineralCost >> 1; }

} // end namespace entity
} // end namespace harvest
