// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_ENTITY_CBUILDINGENTITY_H
#define HARVEST_ENTITY_CBUILDINGENTITY_H

#include "CHarvestEntity.h"
#include "ox/lua/lunar.h"

namespace harvest {
namespace entity {

class CBuildingEntity;

//! A building as Lua scripts see it, bound with Lunar.
class CBuildingLuaInfo
{
public:
    //! Lunar's constructor for objects made from Lua; they have no building.
    CBuildingLuaInfo(lua_State* L);
    //! A view for a building; setEntity names it.
    CBuildingLuaInfo()
    {
    }
    virtual ~CBuildingLuaInfo();

    void setEntity(CBuildingEntity* entity) { Entity = entity; }

    int getId(lua_State* L);
    //! Pushes x and y.
    int getPosition(lua_State* L);
    int getBuildingType(lua_State* L);
    int getDeathParticle(lua_State* L);
    //! Kills the building.
    int remove(lua_State* L);

    static const char className[];
    static Lunar<CBuildingLuaInfo>::RegType methods[];

private:
    CBuildingEntity* Entity;
};

//! Base of the player's buildings. It adds no virtual functions.
class CBuildingEntity : public CEntity
{
public:
    //! Buildings get a Lua view when scripts run: with a Lua manager or in creative mode.
    CBuildingEntity(int id, int type, float x, float y);
    virtual ~CBuildingEntity();

    //! The particle a building of an entity type explodes into.
    static const char* getDeathParticleName(int entityType);
    //! The building id: the construction site, a creative building's own or the buildable item's.
    const char* getBuildingType();
    CBuildingLuaInfo* getLuaInfo() { return LuaInfo; }

protected:
    CBuildingLuaInfo* LuaInfo;
};

} // end namespace entity
} // end namespace harvest

#endif
