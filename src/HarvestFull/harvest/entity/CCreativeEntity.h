// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_ENTITY_CCREATIVEENTITY_H
#define HARVEST_ENTITY_CCREATIVEENTITY_H

#include "CBuildingEntity.h"

namespace harvest {
namespace entity {

class CCreativeEntity;

//! Lua's controls for a creative building and a registry-held private state table.
class CCreativeLuaState
{
public:
    CCreativeLuaState(lua_State* L);
    virtual ~CCreativeLuaState();
    void setEntity(CCreativeEntity* entity) { Entity = entity; }
    int getId(lua_State* L);
    int setRenderColorF(lua_State* L);
    int setProgress(lua_State* L);
    int getPosition(lua_State* L);
    int setWantsEnergy(lua_State* L);
    int getEnergyLevel(lua_State* L);
    int setEnergyLevel(lua_State* L);
    int clearEnergyLevel(lua_State* L);
    int getDeathParticle(lua_State* L);
    int remove(lua_State* L);
    int setSprite(lua_State* L);
    int getPrivates(lua_State* L);
    void writeLuaPrivates(ox::io::IWriteFile* file);
    void readLuaPrivates(ox::io::IReadFile* file, int version);
    static const char className[];
    static Lunar<CCreativeLuaState>::RegType methods[];
private:
    CCreativeEntity* Entity;
    int Privates;
};

//! A creative-mode building whose behavior is supplied by Lua.
//! Private member names are ours; public names and the layout are from the native builds.
class CCreativeEntity : public CBuildingEntity
{
    friend class CCreativeLuaState;
public:
    CCreativeEntity(float x, float y, const char* id);
    virtual ~CCreativeEntity();
    void loadBuildingData(const char* id);
    void loadSprite();
    const wchar_t* getBuildingName() const;
    virtual float getCollisionSize() const;
    virtual int updateLogic(float frameDelta);
    virtual void updateSprite(float frameDelta);
    const char* getBuildingId();
    CCreativeLuaState* getCreativeLuaState() { return CreativeLuaState; }
    virtual int onSpark(CSparkEntity* spark);
    virtual bool wantsSpark();
    virtual void render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort);
    virtual ox::video::ISpriteAnimationState* getCurrentDisplaySprite();
    virtual ox::core::CString<wchar_t> getInfoString();
    virtual ox::core::CString<wchar_t> getMiniStatString();
    virtual void writeEntityData(ox::io::IWriteFile* file);
    virtual void readEntityData(ox::io::IReadFile* file, int version);
    virtual void renderGroundLayer(const ox::core::CPosition2d<float>& camera,
        const ox::core::CRect<int>& viewPort) {}
    virtual int getSellValue() const;
private:
    float Energy;
    ox::video::ISpriteAnimationState* Sprite;
    ox::core::CString<char> BuildingId;
    ox::core::CString<wchar_t> BuildingName;
    ox::core::CString<char> SpritePackage;
    ox::core::CString<char> SpriteName;
    float CollisionSize;
    float Range;
    int MineralCost;
    float Progress;
    bool WantsEnergy;
    CCreativeLuaState* CreativeLuaState;
    ox::core::CString<char> StateName;
};

} // end namespace entity
} // end namespace harvest

#endif
