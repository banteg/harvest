// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_ENTITY_CALIENENTITY_H
#define HARVEST_ENTITY_CALIENENTITY_H

#include "CHarvestEntity.h"
#include "ox/entity/COxEntityManager.h"
#include "ox/core/CVector2d.h"
#include "ox/lua/lunar.h"

namespace harvest {
namespace entity {

//! Localization keys of the alien types; the last five are unused placeholders.
static const wchar_t* const ALIEN_KEY_NAMES[] =
{
    L"aliennames:default",
    L"aliennames:shielder",
    L"aliennames:tiny",
    L"aliennames:summoner",
    L"aliennames:looker",
    L"aliennames:miner",
    L"aliennames:stealer",
    L"aliennames:magneto",
    L"aliennames:mega",
    L"aliennames:asdf",
    L"aliennames:asdf",
    L"aliennames:asdf",
    L"aliennames:asdf",
    L"aliennames:asdf"
};

class CAlienEntity;

//! Lua's view of an alien. The member names in the reconstructed layout are ours.
class CAlienLuaInfo
{
public:
    CAlienLuaInfo() {}
    CAlienLuaInfo(lua_State* L);
    virtual ~CAlienLuaInfo();
    void setEntity(CAlienEntity* entity) { Entity = entity; }
    int remove(lua_State* L);
    int getId(lua_State* L);
    int getAlienType(lua_State* L);
    int getPosition(lua_State* L);
    int dealDamage(lua_State* L);
    int applyForce(lua_State* L);
    int setPosition(lua_State* L);
    int getDeathParticle(lua_State* L);
    int setTargetBuilding(lua_State* L);
    static const char className[];
    static Lunar<CAlienLuaInfo>::RegType methods[];
private:
    CAlienEntity* Entity;
};

const wchar_t* getAlienName(int alienType);
void setAlienName(int alienType, const wchar_t* name);

//! An alien attacking the player.
class CAlienEntity : public CEntity
{
public:
    CAlienEntity(float x, float y, int alienType);
    virtual ~CAlienEntity();
    CAlienLuaInfo* getLuaInfo() { return LuaInfo; }

    virtual void writeEntityData(ox::io::IWriteFile* file);
    virtual void readEntityData(ox::io::IReadFile* file, int version);
    virtual int onSpark(CSparkEntity* spark) { return 0; }
    virtual bool wantsSpark() { return false; }
    virtual void updateSprite(float frameDelta);
    virtual int updateLogic(float frameDelta);
    virtual void render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort);
    virtual void renderGroundLayer(const ox::core::CPosition2d<float>& camera,
        const ox::core::CRect<int>& viewPort);

    //! Applies damage and knockback; returns true when the alien dies.
    bool dealDamage(float& damage, const ox::core::CPosition2d<float>& source, float force, int weapon);

    //! Movement destination at Linux amd64 offset 0x48; the accessor name is ours.
    const ox::core::CVector3d<float>& getMovementTarget() const { return MovementTarget; }

    int getAlienType() const { return AlienType; }
    static const char* getDeathParticleName(int alienType);

private:
    void replaceWithJammerSprite();
    void replaceWithTinySprites();
    void replaceWithSummonerSprites();
    void replaceWithLookerSprites();
    void replaceWithMinerSprites();
    void replaceWithStealerSprites();
    void replaceWithMagnetoSprites();
    void replaceWithMegaSprites();
    void updateMilkyMovement(float frameDelta, ox::core::CVector3d<float>& target,
        ox::core::CVector2d<float>& movement);
    void placeBuildingParticle(ox::entity::COxEntity* building);
    void updateTinyMovement(float frameDelta, ox::core::CVector3d<float>& target,
        ox::core::CVector2d<float>& movement);
    void updateSummonerMovement(float frameDelta, ox::core::CVector3d<float>& target,
        ox::core::CVector2d<float>& movement);
    void updateLookerMovement(float frameDelta, ox::core::CVector3d<float>& target,
        ox::core::CVector2d<float>& movement);
    void locateTargetBuilding();
    void updateMinerMovement(float frameDelta, ox::core::CVector3d<float>& target,
        ox::core::CVector2d<float>& movement);
    void updateStealerMovement(float frameDelta, ox::core::CVector3d<float>& target,
        ox::core::CVector2d<float>& movement);
    void updateMagnetoMovement(float frameDelta, ox::core::CVector3d<float>& target,
        ox::core::CVector2d<float>& movement);
    void updateMegaMovement(float frameDelta, ox::core::CVector3d<float>& target,
        ox::core::CVector2d<float>& movement);
    static float m_nextChantTime;
    static int m_currentChantLine;
    friend class CAlienLuaInfo;

    CAlienLuaInfo* LuaInfo;
    float SpawnCooldown;
public:
    //! Directly assigned by the campaign starting-entity setup.
    ox::entity::SEntityReference Target;
private:
    ox::core::CVector3d<float> MovementTarget;
    float Health;
    ox::core::CVector2d<float> Speed;
    int AlienType;
    float Angle;
    float TargetAngle;
    int SpriteIndex;
    ox::video::IParticleState* Particle;
    char Chant;
    bool Invisible;
    ox::core::CVector3d<float> JumpSpeed;
    bool SummonerCharging;
    bool AnimationDone;
    bool PendingSummon;
    bool MinerLanding;
    float StateTimer;
    float StateAngle;
    signed char EatCount;
    signed char HoggerAttached;
    bool State96;
    bool State97;
    ox::video::ISpriteAnimationState* ShieldSprite;
    ox::video::ISpriteAnimationState* Sprites[74];
};

} // end namespace entity
} // end namespace harvest

#endif
