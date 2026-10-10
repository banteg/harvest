// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include <vector>
#include "ox/gui/IGUIFont.h"
#include "harvest/settings/CSystemConfig.h"
#include "harvest/game/CWorld.h"
#include "harvest/entity/CAlienEntity.h"
#include "harvest/entity/CBuildingEntity.h"
#include "harvest/entity/CEntityManager.h"
#include "harvest/entity/CMineralsEntity.h"
#include "harvest/game/CLuaManager.h"
#include "ox/algo/CRand.h"
#include "ox/core/CBasic.h"
#include "ox/core/CMath.h"
#include "ox/event/IEventReceiver.h"
#include "harvest/entity/CBuildableItems.h"
#include "harvest/game/CStatistics.h"
#include "ox/io/CHelpIO.h"
#include "ox/video/IParticleState.h"
#include "ox/video/IParticlePackage.h"
#include "harvest/entity/CSparkMoverEntity.h"
#include "ox/video/ISpritePackage.h"

namespace harvest {
namespace entity {

float CAlienEntity::m_nextChantTime;
int CAlienEntity::m_currentChantLine;
static ox::core::CString<wchar_t> g_alienNames[14];

const char CAlienLuaInfo::className[] = "CAlienLuaInfo";
Lunar<CAlienLuaInfo>::RegType CAlienLuaInfo::methods[] =
{
    { "getId", &CAlienLuaInfo::getId },
    { "getAlienType", &CAlienLuaInfo::getAlienType },
    { "getPosition", &CAlienLuaInfo::getPosition },
    { "dealDamage", &CAlienLuaInfo::dealDamage },
    { "applyForce", &CAlienLuaInfo::applyForce },
    { "setPosition", &CAlienLuaInfo::setPosition },
    { "getDeathParticle", &CAlienLuaInfo::getDeathParticle },
    { "remove", &CAlienLuaInfo::remove },
    { "setTargetBuilding", &CAlienLuaInfo::setTargetBuilding },
    { 0, 0 }
};

// These two lookup names are provisional; the native tables and indexed accesses are verified.
static const bool BUILDING_TARGET_TYPES[] =
{ true, true, false, true, true, false, false, true, true, false, false, true, false, true, true, false, true };

static inline ox::core::CPosition2d<float> position2d(const ox::core::CVector3d<float>& v)
{
    return ox::core::CPosition2d<float>(v.X, v.Y);
}

static const bool FLYING_IMMUNITY[] = { true, true, false, false, true, true, false };

static const float ALIEN_DAMAGE_MODIFIERS[14][7] =
{
    { 1, 1, 1, 1, 1, 1, 1 },
    { 1, .5f, .5f, 1, .5f, .2f, 1 },
    { 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, .2f, 1 },
    { 1, 1, 1, 1, 1, 1, 1 },
    { 1, 2, 2, 1, 2, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1 },
    { .6f, 1.5f, .6f, .6f, .6f, .6f, 1 },
    { 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1 }
};

void setAlienName(int alienType, const wchar_t* name)
{
    g_alienNames[alienType] = ox::core::CString<wchar_t>(name);
}

const wchar_t* getAlienName(int alienType)
{
    return g_alienNames[alienType].c_str();
}

CAlienEntity::CAlienEntity(float x, float y, int alienType)
    : CEntity(g_nextEntityId++, 6, x, y), Health(100.0f), Speed(0, 0), AlienType(alienType),
      Angle(0), SpriteIndex(0), Particle(0), Chant(0), Invisible(false),
      SummonerCharging(false), AnimationDone(false), PendingSummon(false), MinerLanding(false),
      StateTimer(0), EatCount(0), HoggerAttached(0), State96(false), State97(false),
      ShieldSprite(0)
{
    for (int i = 0; i < 74; ++i)
        Sprites[i] = 0;
    if (game::gp_luaManager)
    {
        LuaInfo = new CAlienLuaInfo();
        LuaInfo->setEntity(this);
    }
    else
        LuaInfo = 0;
    switch (AlienType)
    {
    case 1: replaceWithJammerSprite(); Health = 200.0f; break;
    case 2: replaceWithTinySprites(); Health = 10.0f; break;
    case 3: replaceWithSummonerSprites(); Health = 105.0f; break;
    case 4: replaceWithLookerSprites(); Health = 60.0f; break;
    case 5: replaceWithMinerSprites(); Health = 150.0f; break;
    case 6: replaceWithStealerSprites(); break;
    case 7: replaceWithMagnetoSprites(); break;
    case 8: replaceWithMegaSprites(); Health = 155.0f; break;
    default: Sprites[0] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("Alien")); break;
    }
    SpawnCooldown = (ox::algo::CRand::rand() % 2000) * 0.001f;
}

void CAlienEntity::replaceWithJammerSprite()
{
    if (Sprites[0])
        Sprites[0]->remove();
    Sprites[0] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienJammer"));
    if (!ShieldSprite)
    {
        ShieldSprite = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienShield"));
        ShieldSprite->setFlag(1, true);
    }
}

void CAlienEntity::replaceWithTinySprites()
{
    if (Sprites[0])
        Sprites[0]->remove();
    Sprites[0] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0000"));
    Sprites[1] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0001"));
    Sprites[2] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0002"));
    Sprites[3] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0003"));
    Sprites[4] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0004"));
    Sprites[5] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0005"));
    Sprites[6] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0006"));
    Sprites[7] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0007"));
    Sprites[8] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0008"));
    Sprites[9] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0009"));
    Sprites[10] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0010"));
    Sprites[11] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0011"));
    Sprites[12] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0012"));
    Sprites[13] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0013"));
    Sprites[14] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0014"));
    Sprites[15] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0015"));
    Sprites[16] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0016"));
    Sprites[17] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0017"));
    Sprites[18] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0018"));
    Sprites[19] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0019"));
    Sprites[20] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0020"));
    Sprites[21] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0021"));
    Sprites[22] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0022"));
    Sprites[23] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0023"));
    Sprites[24] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0024"));
    Sprites[25] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0025"));
    Sprites[26] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0026"));
    Sprites[27] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0027"));
    Sprites[28] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0028"));
    Sprites[29] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienTiny0029"));
}

void CAlienEntity::replaceWithSummonerSprites()
{
    if (Sprites[0])
        Sprites[0]->remove();
    Sprites[0] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienSummoner"));
    Sprites[1] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienSummonerOn"));
}

void CAlienEntity::replaceWithLookerSprites()
{
    if (Sprites[0])
        Sprites[0]->remove();
    Sprites[0] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0000"));
    Sprites[1] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0001"));
    Sprites[2] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0002"));
    Sprites[3] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0003"));
    Sprites[4] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0004"));
    Sprites[5] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0005"));
    Sprites[6] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0006"));
    Sprites[7] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0007"));
    Sprites[8] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0008"));
    Sprites[9] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0009"));
    Sprites[10] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0010"));
    Sprites[11] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0011"));
    Sprites[12] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0012"));
    Sprites[13] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0013"));
    Sprites[14] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0014"));
    Sprites[15] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0015"));
    Sprites[16] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0016"));
    Sprites[17] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0017"));
    Sprites[18] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0018"));
    Sprites[19] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0019"));
    Sprites[20] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0020"));
    Sprites[21] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0021"));
    Sprites[22] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0022"));
    Sprites[23] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0023"));
    Sprites[24] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0024"));
    Sprites[25] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0025"));
    Sprites[26] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0026"));
    Sprites[27] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0027"));
    Sprites[28] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0028"));
    Sprites[29] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0029"));
    Sprites[30] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0030"));
    Sprites[31] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0031"));
    Sprites[32] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0032"));
    Sprites[33] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0033"));
    Sprites[34] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0034"));
    Sprites[35] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0035"));
    Sprites[36] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0036"));
    Sprites[37] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0037"));
    Sprites[38] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0038"));
    Sprites[39] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0039"));
    Sprites[40] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0040"));
    Sprites[41] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0041"));
    Sprites[42] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0042"));
    Sprites[43] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0043"));
    Sprites[44] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0044"));
    Sprites[45] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0045"));
    Sprites[46] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0046"));
    Sprites[47] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0047"));
    Sprites[48] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0048"));
    Sprites[49] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0049"));
    Sprites[50] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0050"));
    Sprites[51] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0051"));
    Sprites[52] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0052"));
    Sprites[53] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0053"));
    Sprites[54] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0054"));
    Sprites[55] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0055"));
    Sprites[56] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0056"));
    Sprites[57] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0057"));
    Sprites[58] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0058"));
    Sprites[59] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0059"));
    Sprites[60] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0060"));
    Sprites[61] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0061"));
    Sprites[62] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0062"));
    Sprites[63] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0063"));
    Sprites[64] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0064"));
    Sprites[65] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0065"));
    Sprites[66] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0066"));
    Sprites[67] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0067"));
    Sprites[68] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0068"));
    Sprites[69] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0069"));
    Sprites[70] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0070"));
    Sprites[71] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienLooker0071"));
}

void CAlienEntity::replaceWithMinerSprites()
{
    if (Sprites[0])
        Sprites[0]->remove();
    Sprites[0] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienHogger"));
    Sprites[1] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienHoggerLand"));
    Sprites[2] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienHoggerEat"));
    Sprites[3] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienHoggerLiftoff"));
}

void CAlienEntity::replaceWithStealerSprites()
{
    if (Sprites[0])
        Sprites[0]->remove();
    Sprites[0] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("AlienSparker"));
}

void CAlienEntity::replaceWithMagnetoSprites()
{
    if (Sprites[0])
        Sprites[0]->remove();
    Sprites[0] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0000"));
    Sprites[1] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0001"));
    Sprites[2] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0002"));
    Sprites[3] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0003"));
    Sprites[4] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0004"));
    Sprites[5] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0005"));
    Sprites[6] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0006"));
    Sprites[7] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0007"));
    Sprites[8] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0008"));
    Sprites[9] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0009"));
    Sprites[10] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0010"));
    Sprites[11] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0011"));
    Sprites[12] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0012"));
    Sprites[13] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0013"));
    Sprites[14] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0014"));
    Sprites[15] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0015"));
    Sprites[16] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0016"));
    Sprites[17] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0017"));
    Sprites[18] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0018"));
    Sprites[19] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0019"));
    Sprites[20] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0020"));
    Sprites[21] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0021"));
    Sprites[22] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0022"));
    Sprites[23] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0023"));
    Sprites[24] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0024"));
    Sprites[25] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0025"));
    Sprites[26] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0026"));
    Sprites[27] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0027"));
    Sprites[28] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0028"));
    Sprites[29] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0029"));
    Sprites[30] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0030"));
    Sprites[31] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0031"));
    Sprites[32] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0032"));
    Sprites[33] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0033"));
    Sprites[34] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0034"));
    Sprites[35] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0035"));
    Sprites[36] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0036"));
    Sprites[37] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0037"));
    Sprites[38] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0038"));
    Sprites[39] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0039"));
    Sprites[40] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0040"));
    Sprites[41] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0041"));
    Sprites[42] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0042"));
    Sprites[43] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0043"));
    Sprites[44] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0044"));
    Sprites[45] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0045"));
    Sprites[46] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0046"));
    Sprites[47] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0047"));
    Sprites[48] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0048"));
    Sprites[49] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0049"));
    Sprites[50] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0050"));
    Sprites[51] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0051"));
    Sprites[52] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0052"));
    Sprites[53] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0053"));
    Sprites[54] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0054"));
    Sprites[55] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0055"));
    Sprites[56] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0056"));
    Sprites[57] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0057"));
    Sprites[58] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0058"));
    Sprites[59] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0059"));
    Sprites[60] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0060"));
    Sprites[61] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0061"));
    Sprites[62] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0062"));
    Sprites[63] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0063"));
    Sprites[64] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0064"));
    Sprites[65] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0065"));
    Sprites[66] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0066"));
    Sprites[67] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0067"));
    Sprites[68] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0068"));
    Sprites[69] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0069"));
    Sprites[70] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0070"));
    Sprites[71] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainBottom0071"));
    Sprites[72] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainRotor"));
    Sprites[73] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BrainWave"));
}

void CAlienEntity::replaceWithMegaSprites()
{
    if (Sprites[0])
        Sprites[0]->remove();
    Sprites[0] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("MegaAlienCharge"));
    Sprites[1] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("MegaAlienJump"));
    Sprites[2] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("MegaAlienLand"));
    Sprites[3] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("MegaAlienFly"));
    Sprites[4] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("MegaAlienShadow"));
}

CAlienEntity::~CAlienEntity()
{
    if (ShieldSprite)
        ShieldSprite->remove();
    for (int i = 0; i < 74; ++i)
        if (Sprites[i])
            Sprites[i]->remove();
    if (Particle)
        Particle->remove();
    if (LuaInfo)
        delete LuaInfo;
}

void CAlienEntity::updateSprite(float frameDelta)
{
    if (SpawnCooldown > 0)
        return;
    if (AlienType == 3)
    {
        if (Sprites[SpriteIndex]->update(frameDelta))
            AnimationDone = true;
    }
    else if (AlienType == 5)
    {
        if (Sprites[SpriteIndex]->update(frameDelta))
        {
            switch (SpriteIndex)
            {
            case 0:
                if (MinerLanding)
                {
                    SpriteIndex = 1;
                    Sprites[1]->reset();
                    MinerLanding = false;
                }
                break;
            case 1:
                SpriteIndex = 2;
                Sprites[2]->reset();
                EatCount = 0;
                break;
            case 2:
                if (!Target.Entity)
                {
                    SpriteIndex = 3;
                    Sprites[3]->reset();
                    HoggerAttached = 0;
                }
                else
                {
                    gp_entityManager->updateReference(Target, 0, false);
                    if (Target.Entity && ((CEntity*)Target.Entity)->getEntityType() == 5)
                    {
                        ++EatCount;
                        if (EatCount >= 9)
                        {
                            SpriteIndex = 3;
                            Sprites[3]->reset();
                        }
                        ((CMineralsEntity*)Target.Entity)->withdrawAmount(1);
                        ((CMineralsEntity*)Target.Entity)->setHogStatus(-1);
                        Health = ox::core::min_(Health + 100.0f, 150.0f);
                        HoggerAttached = 0;
                        Target.Entity = 0;
                        Target.Id = -1;
                        gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y + 1.0f,
                            3.0f, 0, "HoggerHeal"), 4);
                    }
                    else if (Target.Entity && !((CEntity*)Target.Entity)->isKilled())
                    {
                        ++EatCount;
                        if (EatCount >= 9)
                        {
                            SpriteIndex = 3;
                            Sprites[3]->reset();
                            ((CEntity*)Target.Entity)->killEntity();
                            placeBuildingParticle(Target.Entity);
                            Target.Entity = 0;
                            Target.Id = -1;
                        }
                    }
                }
                break;
            case 3:
                SpriteIndex = 0;
                Sprites[0]->reset();
                break;
            }
        }
    }
    else if (AlienType == 7)
    {
        if (Sprites[72])
            Sprites[72]->update(frameDelta);
        if (Sprites[73])
            Sprites[73]->update(frameDelta);
    }
    else if (AlienType == 8)
    {
        if (Sprites[SpriteIndex]->update(frameDelta))
        {
            switch (SpriteIndex)
            {
            case 0: SpriteIndex = 1; break;
            case 1: SpriteIndex = 3; break;
            case 2: SpriteIndex = 0; break;
            }
            Sprites[SpriteIndex]->reset();
        }
    }
    else if (Sprites[0])
        Sprites[0]->update(frameDelta);
}

void CAlienEntity::placeBuildingParticle(ox::entity::COxEntity* building)
{
    const char* particle = CBuildingEntity::getDeathParticleName(((CEntity*)building)->getEntityType());
    gp_entityManager->appendEntity(new CParticleEntity(building->getPosition().X,
        building->getPosition().Y, 1.0f, 0, particle), 4);
    ox::event::SEvent event;
    event.EventType = ox::event::EET_USER_EVENT;
    event.UserEvent.UserData1 = 8;
    event.UserEvent.UserData2 = (int)building->getPosition().X;
    event.UserEvent.UserData3 = (int)building->getPosition().Y;
    event.UserEvent.UserPointer = 0;
    ox::event::gp_subscriberList->OnEvent(event);
    if (game::gp_luaManager)
    {
        const char* type = gp_buildableItems->getEntityIdForEntityInstance((CEntity*)building);
        game::gp_luaManager->hookBuildingDestroyed(type, building->getPosition().X, building->getPosition().Y, this);
    }
}

const char* CAlienEntity::getDeathParticleName(int alienType)
{
    switch (alienType)
    {
    case 1: return "AlienDeathShielder";
    case 2: return "AlienDeathSmall";
    case 3: return "AlienDeathLarge";
    case 4: return "AlienDeathLooker";
    case 5: return "AlienDeathHarvester";
    case 6: return "AlienDeathSparker";
    case 7: return "AlienDeathBrain";
    case 8: return "AlienDeathMega";
    }
    return "AlienDeath";
}

int CAlienEntity::updateLogic(float frameDelta)
{
    if (Health <= 0)
    {
        ox::core::CVector3d<float> speed(Speed.X, Speed.Y, 1.0f);
        const char* particle = getDeathParticleName(AlienType);
        if (AlienType == 8 && Position.Z > 10.0f)
        {
            ox::event::SEvent event;
            event.EventType = ox::event::EET_USER_EVENT;
            event.UserEvent.UserData1 = 21;
            event.UserEvent.UserData2 = 10;
            event.UserEvent.UserData3 = 0;
            event.UserEvent.UserPointer = 0;
            ox::event::gp_subscriberList->OnEvent(event);
        }
        ox::core::CVector3d<float>* speedPointer = &speed;
        gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y,
            Position.Z + 3.0f, speedPointer, particle), 4);
        if (game::gp_luaManager)
            game::gp_luaManager->hookAlienDeath(AlienType, Position);
        return 1;
    }
    ox::core::CVector3d<float> oldPosition = Position;
    if (gp_entityManager->hasBuildingListChanged() && Target.Entity)
    {
        int grid = gp_entityManager->calculateGridPosition(MovementTarget);
        if (grid >= 0)
        {
            Target.Entity = gp_entityManager->locateEntityInGrid(Target.Id, grid, 0);
            if (Target.Entity)
                Target.UpdateCounter = gp_entityManager->getUpdateCounter();
        }
        else
            gp_entityManager->updateReference(Target, 0, true);
    }
    else
        gp_entityManager->updateReference(Target, 0, true);
    if (!Target.Entity)
    {
        if (AlienType != 5)
        {
            if (AlienType == 7)
            {
                SummonerCharging = false;
                PendingSummon = false;
            }
        }
        else
        {
            if (SpriteIndex != 0 && SpriteIndex != 3)
            {
                SpriteIndex = 3;
                Sprites[3]->reset();
            }
        }
        locateTargetBuilding();
    }
    ox::core::CVector3d<float> target;
    if (!Target.Entity)
    {
        if (ox::algo::CRand::rand() % 100 == 0)
            return 1;
        if (game::gp_world)
        {
            const ox::core::CRect<float>& field = game::gp_world->getVisibleGameFieldSize();
            ox::core::CPosition2d<float> center((field.UpperLeftCorner.X + field.LowerRightCorner.X) * .5f,
                (field.UpperLeftCorner.Y + field.LowerRightCorner.Y) * .5f);
            float angle = Id * 10.0f * .0174532905f;
            double targetXOffset = cos((double)angle) * 450.0;
            target.X = targetXOffset + center.X;
            target.Y = sin((double)angle) * 450.0 + center.Y;
        }
    }
    else
        target = Target.Entity->getPosition();
    if (SpawnCooldown > 0)
    {
        SpawnCooldown -= frameDelta;
        return 0;
    }
    if (Invisible)
    {
        target.X = JumpSpeed.X;
        target.Y = JumpSpeed.Y;
        JumpSpeed.Z += frameDelta;
        if (JumpSpeed.Z > 30.0f)
            Health = 0;
    }
    if (Chant)
    {
        m_nextChantTime -= frameDelta;
        if (m_nextChantTime < -5.0f)
        {
            m_nextChantTime = (ox::algo::CRand::rand() % 5000) * .001f + 5.0f;
            int currentChantLine = ox::algo::CRand::rand() % 10;
            m_currentChantLine = currentChantLine;
        }
    }
    ox::core::CVector2d<float> movement(target.X - Position.X, target.Y - Position.Y);
    if (game::gp_world->Planet == 1)
        game::gp_world->applyWind(Position, Speed, frameDelta);
    MovementTarget = target;
    switch (AlienType)
    {
    case 0: updateMilkyMovement(frameDelta, target, movement); break;
    case 2: updateTinyMovement(frameDelta, target, movement); break;
    case 3: updateSummonerMovement(frameDelta, target, movement); break;
    case 4: updateLookerMovement(frameDelta, target, movement); break;
    case 5: updateMinerMovement(frameDelta, target, movement); break;
    case 6: updateStealerMovement(frameDelta, target, movement); break;
    case 7: updateMagnetoMovement(frameDelta, target, movement); break;
    case 8: updateMegaMovement(frameDelta, target, movement); break;
    default:
        movement.normalize();
        movement *= frameDelta * 10.0f;
        break;
    }
    movement += Speed * frameDelta;
    if (AlienType != 8)
    {
        float y = Position.Y;
        ox::core::CPosition2d<float> position(Position.X + movement.X, y + movement.Y);
        if (game::gp_world->mayMoveHere(position))
        {
            Position.X = position.X;
            Position.Y = position.Y;
        }
        else if (!Invisible)
        {
            float angle = game::gp_world->getCollisionTangent(position);
            ox::core::CVector2d<float> tangent(cosf(angle), sinf(angle));
            ox::core::CVector2d<float> direction(MovementTarget.X - Position.X, MovementTarget.Y - Position.Y);
            direction.normalize();
            if (direction.X * tangent.Y - direction.Y * tangent.X > 0)
                angle -= 1.2566371f;
            else
                angle += 1.2566371f;
            JumpSpeed.X = position.X + cos((double)angle) * 10.0;
            double jumpSpeedYOffset = sin((double)angle) * 10.0;
            JumpSpeed.Y = position.Y + jumpSpeedYOffset;
            JumpSpeed.Z = 0;
            Invisible = true;
        }
    }
    else
    {
        Position.X += movement.X;
        Position.Y += movement.Y;
    }
    gp_entityManager->updateGridEntity(this, oldPosition, 1);
    if (AlienType != 2)
        Speed *= ox::core::max_(0.0f, 1.0f - frameDelta * 5.0f);
    else
        Speed *= ox::core::max_(0.0f, 1.0f - frameDelta * .5f);
    if (AlienType != 8 && AlienType != 5 && !Invisible && Target.Entity)
    {
        float dx = target.X - Position.X;
        float dy = target.Y - Position.Y;
        if (ox::core::abs_(dx) < 5.0f && ox::core::abs_(dy) < 5.0f)
        {
            Target.Entity->killEntity();
            placeBuildingParticle(Target.Entity);
            Target.Entity = 0;
            Target.Id = -1;
            if (AlienType == 2)
                return 1;
        }
    }
    else if (Invisible)
    {
        float dx = target.X - Position.X;
        float dy = target.Y - Position.Y;
        if (ox::core::abs_(dx) < 5.0f && ox::core::abs_(dy) < 5.0f)
            Invisible = false;
    }
    if (ShieldSprite)
    {
        if (ShieldSprite->update(frameDelta))
            ShieldSprite->setFlag(1, true);
    }
    return 0;
}

void CAlienEntity::locateTargetBuilding()
{
    if (Target.Entity)
        return;
    if (HoggerAttached == 1)
    {
        float bestDistance = 1000000000.0f;
        const std::list<ox::entity::COxEntity*>& buildings = gp_entityManager->getEntityList(0);
        for (std::list<ox::entity::COxEntity*>::const_iterator it = buildings.begin(); it != buildings.end(); it++)
        {
            if (((CEntity*)*it)->getEntityType() == 5 && !((CEntity*)*it)->isKilled() &&
                ((CMineralsEntity*)*it)->getHoggerId() == -1)
            {
                float x = (*it)->getPosition().X - Position.X;
                float y = (*it)->getPosition().Y - Position.Y;
                float distance = x * x + y * y;
                if (distance < bestDistance)
                {
                    Target.Entity = *it;
                    Target.Id = Target.Entity->getId();
                    Target.UpdateCounter = gp_entityManager->getUpdateCounter();
                    bestDistance = distance;
                }
            }
        }
        return;
    }
    std::vector<ox::entity::COxEntity*> targets;
    bool availableLink = false;
    const std::list<ox::entity::COxEntity*>& buildings = gp_entityManager->getEntityList(0);
    for (std::list<ox::entity::COxEntity*>::const_iterator it = buildings.begin(); it != buildings.end(); it++)
    {
        if (BUILDING_TARGET_TYPES[((CEntity*)*it)->getEntityType()] && !((CEntity*)*it)->isKilled())
        {
            targets.push_back(*it);
            if (((CEntity*)*it)->getEntityType() == 1 && !((CSparkMoverEntity*)*it)->isAlienWaypointed())
                availableLink = true;
        }
    }
    if (!targets.empty())
    {
        if (availableLink && AlienType == 6)
        {
            float bestDistance = 1000000000.0f;
            for (unsigned int i = 0; i < targets.size(); ++i)
            {
                if (((CEntity*)targets[i])->getEntityType() == 1 &&
                    !((CSparkMoverEntity*)targets[i])->isAlienWaypointed())
                {
                    float x = targets[i]->getPosition().X - Position.X;
                    float y = targets[i]->getPosition().Y - Position.Y;
                    float distance = x * x + y * y;
                    if (distance < bestDistance)
                    {
                        Target.Entity = targets[i];
                        Target.Id = Target.Entity->getId();
                        Target.UpdateCounter = gp_entityManager->getUpdateCounter();
                        bestDistance = distance;
                    }
                }
            }
        }
        if (!Target.Entity)
        {
            ox::entity::COxEntity* first = targets[ox::algo::CRand::rand() % targets.size()];
            ox::entity::COxEntity* second = targets[ox::algo::CRand::rand() % targets.size()];
            float firstDistance = ox::core::CMath::getSquaredDistance(
                position2d(first->getPosition()),
                ox::core::CPosition2d<float>(Position.X, Position.Y));
            float secondDistance = ox::core::CMath::getSquaredDistance(
                position2d(second->getPosition()),
                ox::core::CPosition2d<float>(Position.X, Position.Y));
            float distance;
            if (firstDistance < secondDistance)
            {
                Target.Entity = first;
                distance = firstDistance;
            }
            else
            {
                Target.Entity = second;
                distance = secondDistance;
            }
            //! Widen the accepted range each time a random pick is not closer.
            float range = 1500.0f;
            int attempts = 10;
            while (distance > range * range && attempts > 0)
            {
                ox::entity::COxEntity* candidate = targets[ox::algo::CRand::rand() % targets.size()];
                float candidateDistance = ox::core::CMath::getSquaredDistance(
                    position2d(candidate->getPosition()),
                    ox::core::CPosition2d<float>(Position.X, Position.Y));
                if (candidateDistance < distance)
                {
                    Target.Entity = candidate;
                    distance = candidateDistance;
                }
                else
                    range += 500.0f;
                --attempts;
            }
        }
        if (Target.Id == -1 && AlienType == 4)
            TargetAngle = ox::core::CMath::getAngleIY(ox::core::CPosition2d<float>(Position.X, Position.Y),
                position2d(Target.Entity->getPosition()));
        Target.Id = Target.Entity->getId();
        Target.UpdateCounter = gp_entityManager->getUpdateCounter();
    }
}

void CAlienEntity::updateMilkyMovement(float frameDelta, ox::core::CVector3d<float>& target,
    ox::core::CVector2d<float>& movement)
{
    movement.normalize();
    movement *= frameDelta * 10.0f;
}

void CAlienEntity::updateTinyMovement(float frameDelta, ox::core::CVector3d<float>& target,
    ox::core::CVector2d<float>& movement)
{
    float angle = ox::core::CMath::getAngleIY(ox::core::CPosition2d<float>(Position.X, Position.Y),
        ox::core::CPosition2d<float>(target.X, target.Y));
    if (angle < 0) angle += 6.28318548f;
    else if (angle > 6.28318548f) angle -= 6.28318548f;
    float step = 1.5f;
    if (ox::core::CMath::clockWiseClosestIY(Angle, angle))
    {
        Angle += step * frameDelta;
        if (!ox::core::CMath::clockWiseClosestIY(Angle, angle))
            Angle = angle;
    }
    else
    {
        Angle += -step * frameDelta;
        if (ox::core::CMath::clockWiseClosestIY(Angle, angle))
            Angle = angle;
    }
    while (Angle < 0) Angle += 6.28318548f;
    while (Angle > 6.28318548f) Angle -= 6.28318548f;
    if (movement.X * movement.X + movement.Y * movement.Y > 625.0f)
    {
        Speed.X += cos((double)Angle) * 50.0 * frameDelta;
        Speed.Y += sin((double)Angle) * 50.0 * frameDelta;
    }
    else
    {
        Speed.X += cos((double)Angle) * 10.0 * frameDelta;
        Speed.Y += sin((double)Angle) * 10.0 * frameDelta;
    }
    movement.X = 0;
    movement.Y = 0;
    SpriteIndex = 0;
    if (Angle > 0.104719758f && Angle < 6.178465843200684f)
        SpriteIndex = 29 - (int)((Angle - 0.104719758f) / .209439516f);
    ox::core::CVector3d<float> position(Position.X + cos((double)Angle) * -8.0,
        Position.Y + sin((double)Angle) * -5.0, .1f);
    if (Particle && !Particle->update(frameDelta, position))
    {
        Particle->remove();
        Particle = 0;
    }
}

void CAlienEntity::updateSummonerMovement(float frameDelta, ox::core::CVector3d<float>& target,
    ox::core::CVector2d<float>& movement)
{
    if (StateTimer > 0)
    {
        StateTimer -= frameDelta;
        if (StateTimer <= 0)
        {
            if (PendingSummon)
            {
                PendingSummon = false;
                for (int i = 0; i < 3; ++i)
                {
                    float angle = i * 2.09439516f + StateAngle;
                    double sine = sin((double)angle);
                    double cosine = cos((double)angle);
                    ox::core::CPosition2d<float> position;
                    position.X = Position.X + cosine * 60.0;
                    position.Y = Position.Y + sine * 50.0;
                    if (game::gp_world->mayPlaceObjectHere(position, false))
                    {
                        CAlienEntity* alien = new CAlienEntity(position.X, position.Y, 2);
                        alien->Speed.X = cosine * 120.0;
                        alien->Speed.Y = sine * 120.0;
                        alien->Angle = angle;
                        alien->SpawnCooldown = 0;
                        if (i == 0)
                            alien->Target.Id = Target.Id;
                        gp_entityManager->appendEntity(alien, 1);
                    }
                }
            }
            else if (SummonerCharging)
            {
                PendingSummon = true;
                StateTimer = 4.0f;
                StateAngle = (ox::algo::CRand::rand() % 1000 * .001f * 2.0f) * 3.14159274f;
                for (int i = 0; i < 3; ++i)
                {
                    float angle = i * 2.09439516f + StateAngle;
                    double sine = sin((double)angle);
                    double cosine = cos((double)angle);
                    float x = Position.X + cosine * 60.0;
                    float y = Position.Y + sine * 50.0;
                    gp_entityManager->appendEntity(new CParticleEntity(x, y, 5.0f, 0, "AlienTeleportation"), 4);
                }
            }
        }
    }
    if (SummonerCharging)
    {
        movement.X = 0;
        movement.Y = 0;
        SpriteIndex = 1;
        MovementTarget = Position;
        if (StateTimer <= 0)
            StateTimer = 3.0f;
    }
    else
    {
        SpriteIndex = 0;
        movement.normalize();
        movement *= frameDelta * 10.0f;
    }
    if (AnimationDone)
    {
        AnimationDone = false;
        CEntity* building = gp_entityManager->findBuildingInRange(
            ox::core::CPosition2d<float>(Position.X, Position.Y), 62500.0f);
        if (!building)
        {
            if (SummonerCharging)
            {
                SummonerCharging = false;
                SpriteIndex = 0;
                Sprites[0]->reset();
            }
        }
        else
        {
            if (!SummonerCharging)
            {
                SummonerCharging = true;
                SpriteIndex = 1;
                Sprites[1]->reset();
                gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y,
                    1.0f, 0, "AlienChargeUp"), 4);
            }
            Target.Entity = building;
            Target.Id = building->getId();
            Target.UpdateCounter = gp_entityManager->getUpdateCounter();
        }
    }
}

void CAlienEntity::updateLookerMovement(float frameDelta, ox::core::CVector3d<float>& target,
    ox::core::CVector2d<float>& movement)
{
    if (TargetAngle < 0) TargetAngle += 6.28318548f;
    else if (TargetAngle > 6.28318548f) TargetAngle -= 6.28318548f;
    float angle = TargetAngle;
    if (angle < 0) angle += 6.28318548f;
    else if (angle > 6.28318548f) angle -= 6.28318548f;
    float step = 1.5f;
    if (ox::core::CMath::clockWiseClosestIY(Angle, angle))
    {
        Angle += step * frameDelta;
        if (!ox::core::CMath::clockWiseClosestIY(Angle, angle))
            Angle = angle;
    }
    else
    {
        Angle += -step * frameDelta;
        if (ox::core::CMath::clockWiseClosestIY(Angle, angle))
            Angle = angle;
    }
    if (Angle < 0) Angle += 6.28318548f;
    else if (Angle > 6.28318548f) Angle -= 6.28318548f;
    movement.normalize();
    movement *= frameDelta * 10.0f;
    SpriteIndex = 0;
    if (Angle > 0.0436332338f && Angle < 6.239552021026611f)
        SpriteIndex = 71 - (int)((Angle - 0.0436332338f) / 0.0872664675f);
}

void CAlienEntity::updateMinerMovement(float frameDelta, ox::core::CVector3d<float>& target,
    ox::core::CVector2d<float>& movement)
{
    if (SpriteIndex != 0)
    {
        movement.X = 0;
        movement.Y = 0;
        return;
    }
    movement.normalize();
    if (HoggerAttached == 1)
        movement *= 25.0f * frameDelta;
    else
        movement *= 15.0f * frameDelta;
    if (HoggerAttached == 0 && Health < 75.0f)
    {
        ox::entity::COxEntity* oldEntity = Target.Entity;
        int oldId = Target.Id;
        Target.Entity = 0;
        Target.Id = -1;
        HoggerAttached = 1;
        locateTargetBuilding();
        if (!Target.Entity)
        {
            HoggerAttached = 2;
            Target.Entity = oldEntity;
            Target.Id = oldId;
            Target.UpdateCounter = 0;
        }
    }
    else if (HoggerAttached == 1)
    {
        if (Target.Entity)
        {
            int hogger = ((CMineralsEntity*)Target.Entity)->getHoggerId();
            if (((CEntity*)Target.Entity)->isKilled() || (hogger >= 0 && hogger != Id))
            {
                Target.Entity = 0;
                Target.Id = -1;
                locateTargetBuilding();
                if (!Target.Entity)
                {
                    HoggerAttached = 2;
                    locateTargetBuilding();
                }
            }
            else
            {
                ox::core::CVector3d<float> position = Target.Entity->getPosition();
                float dx = position.X - Position.X;
                float dy = position.Y - Position.Y;
                if (ox::core::abs_(dx) < 2.0f && ox::core::abs_(dy) < 2.0f)
                {
                    Position = position;
                    Position.Y += 1.0f;
                    ((CMineralsEntity*)Target.Entity)->setHogStatus(Id);
                    MinerLanding = true;
                }
                else
                    MinerLanding = false;
            }
        }
    }
    else if (Target.Entity)
    {
        ox::core::CVector3d<float> position = Target.Entity->getPosition();
        float dx = position.X - Position.X;
        float dy = position.Y - Position.Y;
        if (ox::core::abs_(dx) < 2.0f && ox::core::abs_(dy) < 2.0f)
        {
            Position = position;
            Position.Y += 1.0f;
            MinerLanding = true;
        }
        else
            MinerLanding = false;
        if (StateTimer > 0 && !MinerLanding)
        {
            StateTimer -= frameDelta;
            if (StateTimer <= 0)
            {
                StateTimer += (ox::algo::CRand::rand() % 1000) * .002f + 5.0f;
                float distance = ox::core::CMath::getEstimateDistance(
                    ox::core::CPosition2d<float>(Position.X, Position.Y),
                    ox::core::CPosition2d<float>(position.X, position.Y));
                const std::list<ox::entity::COxEntity*>& buildings = gp_entityManager->getEntityList(0);
                for (std::list<ox::entity::COxEntity*>::const_reverse_iterator it = buildings.rbegin(); it != buildings.rend(); it++)
                {
                    if (BUILDING_TARGET_TYPES[((CEntity*)*it)->getEntityType()] && !((CEntity*)*it)->isKilled())
                    {
                        ox::core::CVector3d<float> candidatePosition = (*it)->getPosition();
                        float candidate = ox::core::CMath::getEstimateDistance(
                            ox::core::CPosition2d<float>(Position.X, Position.Y),
                            ox::core::CPosition2d<float>(candidatePosition.X, candidatePosition.Y));
                        if (candidate < distance && ox::algo::CRand::rand() % 3 == 0)
                        {
                            Target.Entity = *it;
                            Target.Id = Target.Entity->getId();
                            Target.UpdateCounter = gp_entityManager->getUpdateCounter();
                            break;
                        }
                    }
                }
            }
        }
        else
            StateTimer = (ox::algo::CRand::rand() % 1000) * .002f + 5.0f;
    }
}

void CAlienEntity::updateStealerMovement(float frameDelta, ox::core::CVector3d<float>& target,
    ox::core::CVector2d<float>& movement)
{
    bool move = true;
    if (Target.Entity && ((CEntity*)Target.Entity)->getEntityType() == 1 && !Invisible)
    {
        if (((CSparkMoverEntity*)Target.Entity)->isAlienWaypointed() && ((CSparkMoverEntity*)Target.Entity)->getWaypointId() != Id)
        {
            Target.Entity = 0;
            Target.Id = -1;
            locateTargetBuilding();
        }
        else if (!((CSparkMoverEntity*)Target.Entity)->isAlienWaypointed())
        {
            if (ox::core::CMath::getSquaredDistance(ox::core::CPosition2d<float>(Position.X, Position.Y),
                    ox::core::CPosition2d<float>(target.X, target.Y)) < 40000.0f)
            {
                ((CSparkMoverEntity*)Target.Entity)->setSparkTargetId(Id, true);
                if (!Particle)
                    Particle = gp_particlePackage->addNewParticleState(ox::core::CString<char>("StealerRay"));
                move = false;
            }
        }
        else
        {
            ox::core::CVector3d<float> position = Target.Entity->getPosition();
            if (ox::core::CMath::getSquaredDistance(ox::core::CPosition2d<float>(Position.X, Position.Y),
                    ox::core::CPosition2d<float>(position.X, position.Y)) > 44100.0f)
            {
                ((CSparkMoverEntity*)Target.Entity)->setSparkTargetId(-1, false);
                Target.Entity = 0;
                Target.Id = -1;
                locateTargetBuilding();
            }
            else if (Particle && !Particle->update(frameDelta, position))
            {
                Particle->remove();
                Particle = 0;
            }
            move = false;
        }
    }
    if (move)
    {
        movement.normalize();
        movement *= frameDelta * 10.0f;
        if (Particle)
        {
            Particle->remove();
            Particle = 0;
        }
    }
    else
    {
        movement.X = 0;
        movement.Y = 0;
    }
}

void CAlienEntity::updateMagnetoMovement(float frameDelta, ox::core::CVector3d<float>& target,
    ox::core::CVector2d<float>& movement)
{
    movement.normalize();
    movement *= 10.0f * frameDelta;
    Angle = ox::core::CMath::getAngleIY(ox::core::CPosition2d<float>(Position.X, Position.Y),
        ox::core::CPosition2d<float>(target.X, target.Y));
    if (Angle < 0) Angle += 6.28318548f;
    else if (Angle > 6.28318548f) Angle -= 6.28318548f;
    if (!SummonerCharging)
    {
        StateTimer -= frameDelta;
        if (StateTimer <= 0)
        {
            StateTimer = .25f;
            CEntity* building = gp_entityManager->findBuildingInRange(
                ox::core::CPosition2d<float>(Position.X, Position.Y), 62500.0f);
            if (building)
            {
                SummonerCharging = true;
                PendingSummon = true;
                StateAngle = (ox::algo::CRand::rand() % 1000) * .001f * 6.28318548f;
                StateTimer = 2.0f;
                Target.Entity = building;
                Target.Id = building->getId();
                Target.UpdateCounter = gp_entityManager->getUpdateCounter();
                gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y, 1.0f, 0, "BrainOrder"), 4);
            }
        }
    }
    if (PendingSummon)
    {
        movement.X = 0;
        movement.Y = 0;
        StateAngle += 6.28318548f * frameDelta;
        StateTimer -= frameDelta;
        if (StateTimer <= 0)
        {
            PendingSummon = false;
            if (Target.Entity)
            {
                gp_entityManager->appendEntity(new CParticleEntity(Target.Entity->getPosition().X,
                    Target.Entity->getPosition().Y, 1.0f, 0, "BrainBuildingTarget"), 4);
                const std::list<ox::entity::COxEntity*>& aliens = gp_entityManager->getEntityList(1);
                std::list<ox::entity::COxEntity*>::const_iterator end = aliens.end();
                for (std::list<ox::entity::COxEntity*>::const_iterator it = aliens.begin(); it != end; it++)
                {
                    if (((CEntity*)*it)->getEntityType() != 6)
                        continue;
                    CAlienEntity* alien = (CAlienEntity*)*it;
                    if (alien->AlienType == 7)
                        continue;
                    ox::core::CVector3d<float> position = alien->getPosition();
                    if (ox::core::CMath::getSquaredDistance(ox::core::CPosition2d<float>(Position.X, Position.Y),
                            ox::core::CPosition2d<float>(position.X, position.Y)) < 22500.0f)
                    {
                        alien->Target.Entity = Target.Entity;
                        alien->Target.Id = Target.Id;
                        alien->Target.UpdateCounter = Target.UpdateCounter;
                        gp_entityManager->appendEntity(new CParticleEntity(alien->Position.X,
                            alien->Position.Y, 1.0f, 0, "BrainTarget"), 4);
                    }
                }
            }
        }
    }
    SpriteIndex = 0;
    if (Angle > .0436332338f && Angle < 6.239552021026611f)
        SpriteIndex = 71 - (int)((Angle - .0436332338f) / .0872664675f);
}

void CAlienEntity::updateMegaMovement(float frameDelta, ox::core::CVector3d<float>& target,
    ox::core::CVector2d<float>& movement)
{
    Speed.X = 0;
    Speed.Y = 0;
    StateTimer += frameDelta;
    if (Position.Z > 0)
    {
        Position.Z += Angle * frameDelta;
        if (Position.Z <= 0)
        {
            Position.Z = 0;
            SpriteIndex = 2;
            Sprites[2]->reset();
            movement.X = 0;
            movement.Y = 0;
            const std::list<ox::entity::COxEntity*>& aliens = gp_entityManager->getEntityList(1);
            for (std::list<ox::entity::COxEntity*>::const_iterator it = aliens.begin(); it != aliens.end(); it++)
            {
                if ((*it)->getEntityType() == 6)
                {
                    CAlienEntity* alien = (CAlienEntity*)*it;
                    if (alien->AlienType != 8)
                    {
                        float distance = ox::core::CMath::getSquaredDistance(
                            ox::core::CPosition2d<float>(Position.X, Position.Y), position2d(alien->getPosition()));
                        if (distance < 2500.0f)
                        {
                            float damage = (distance / -2500.0f + 1.0f) * 15.0f;
                            alien->dealDamage(damage, ox::core::CPosition2d<float>(Position.X, Position.Y), 8.0f, 5);
                        }
                    }
                }
            }
            bool killed = false;
            const std::list<ox::entity::COxEntity*>& buildings = gp_entityManager->getEntityList(0);
            for (std::list<ox::entity::COxEntity*>::const_iterator it = buildings.begin(); it != buildings.end(); it++)
            {
                if ((*it)->getEntityType() != 5)
                {
                    float distance = ox::core::CMath::getSquaredDistance(ox::core::CPosition2d<float>(Position.X, Position.Y),
                        position2d((*it)->getPosition()));
                    if (distance < 900.0f)
                    {
                        (*it)->killEntity();
                        placeBuildingParticle(*it);
                        killed = true;
                    }
                }
            }
            if (killed)
                gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y, 1.0f, 0, "MegaLandKill"), 4);
            else
                gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y, 1.0f, 0, "MegaLand"), 4);
        }
        else
        {
            Angle -= 700.0f * frameDelta;
            movement.X = TargetAngle * frameDelta;
            movement.Y = StateAngle * frameDelta;
        }
    }
    else if (SpriteIndex == 3)
    {
        Position.Z = ox::core::max_(frameDelta * 400.0f, .01f);
        Angle = frameDelta * -700.0f + 400.0f;
        float length = movement.getLength();
        movement.normalize();
        float speed;
        if (length < 115.0f)
            speed = length / 115.0f * 100.0f;
        else
        {
            ox::core::CPosition2d<float> position;
            position.X = movement.X * 115.0f + Position.X;
            position.Y = movement.Y * 115.0f + Position.Y;
            if (game::gp_world->mayMoveHere(position))
                speed = 100.0f;
            else
            {
                position.X = movement.X * 115.0f * .5f + Position.X;
                position.Y = movement.Y * 115.0f * .5f + Position.Y;
                if (game::gp_world->mayMoveHere(position))
                    speed = 50.0f;
                else
                {
                    position.X = movement.X * 115.0f * 2.0f + Position.X;
                    position.Y = movement.Y * 115.0f * 2.0f + Position.Y;
                    if (game::gp_world->mayMoveHere(position))
                        speed = 200.0f;
                    else
                    {
                        ox::core::CVector2d<float> original = movement;
                        const float ATTEMPT_ANGLES_DEGREES[] = { -20, 20, -45, 45, -90, 90, -135, 135, 180 };
                        bool found = false;
                        for (int i = 0; i < 9; ++i)
                        {
                            movement = original;
                            movement.rotateBy(ATTEMPT_ANGLES_DEGREES[i]);
                            position.X = movement.X * 115.0f + Position.X;
                            position.Y = movement.Y * 115.0f + Position.Y;
                            if (game::gp_world->mayMoveHere(position))
                            {
                                found = true;
                                break;
                            }
                        }
                        if (!found)
                        {
                            movement.X = 0;
                            movement.Y = 0;
                        }
                        speed = 100.0f;
                    }
                }
            }
        }
        TargetAngle = movement.X * speed;
        StateAngle = movement.Y * speed;
        movement.X = 0;
        movement.Y = 0;
        gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y, 1.0f, 0, "MegaJump"), 4);
        StateTimer = 0;
    }
    else
    {
        movement.X = 0;
        movement.Y = 0;
    }
}

bool CAlienEntity::dealDamage(float& damage, const ox::core::CPosition2d<float>& source,
    float force, int weapon)
{
    if (Killed || Health <= 0)
    {
        damage = 0;
        return false;
    }
    // The flying mega alien is immune to these four weapons; the local name is ours.
    if (AlienType == 8 && Position.Z > 10.0f && FLYING_IMMUNITY[weapon])
    {
        damage = 0;
        return false;
    }
    float originalDamage = damage;
    damage *= ALIEN_DAMAGE_MODIFIERS[AlienType][weapon];
    if (AlienType == 1 && ALIEN_DAMAGE_MODIFIERS[1][weapon] < 1.0f)
    {
        ShieldSprite->reset();
        ShieldSprite->setFlag(1, false);
    }
    else if (AlienType == 4 && weapon == 0)
    {
        ox::core::CVector2d<float> direction(source.X - Position.X, source.Y - Position.Y);
        ox::core::CVector2d<float> forward(cosf(Angle), sinf(Angle));
        TargetAngle = ox::core::CMath::getAngleIY(ox::core::CPosition2d<float>(Position.X, Position.Y), source);
        direction.normalize();
        if (direction.X * forward.X + direction.Y * forward.Y > .95f)
        {
            damage = 0;
            return false;
        }
    }
    bool killed = false;
    if (Health > 0 && damage > 0)
    {
        float effective = ox::core::min_(Health, damage);
        Health -= damage;
        if (Health <= 0)
        {
            damage += Health;
            killed = true;
            if (AlienType == 5 && HoggerAttached == 1 && Target.Entity)
            {
                gp_entityManager->updateReference(Target, 0, false);
                if (Target.Entity && ((CEntity*)Target.Entity)->getEntityType() == 5)
                    ((CMineralsEntity*)Target.Entity)->setHogStatus(-1);
            }
        }
        if (game::gp_statistics)
        {
            if (weapon == 0)
                game::gp_statistics->reportAlienStatChange(1, AlienType, effective);
            else if (weapon == 1 || weapon == 3 || weapon == 2)
                game::gp_statistics->reportAlienStatChange(2, AlienType, effective);
            else if (weapon == 4)
                game::gp_statistics->reportAlienStatChange(3, AlienType, effective);
            if (killed)
                game::gp_statistics->reportAlienStatChange(0, AlienType, 1.0f);
        }
    }
    if (AlienType != 5 || SpriteIndex == 0)
    {
        ox::core::CVector2d<float> direction(Position.X - source.X, Position.Y - source.Y);
        direction.normalize();
        Speed += direction * (force * originalDamage);
    }
    return killed;
}

int CAlienEntity::onSpark(CSparkEntity* spark)
{
    return 0;
}

bool CAlienEntity::wantsSpark()
{
    return false;
}

void CAlienEntity::render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort)
{
    if (AlienType == 2)
    {
        renderSprite(camera, viewPort, Sprites[SpriteIndex]);
        if (Particle)
        {
            ox::core::CPosition2d<float> position;
            position.X = Position.X - camera.X + viewPort.UpperLeftCorner.X;
            position.Y = Position.Y - camera.Y + viewPort.UpperLeftCorner.Y;
            Particle->render2D(position, 1.0f);
        }
    }
    else if (AlienType == 3 || AlienType == 4 || AlienType == 5)
        renderSprite(camera, viewPort, Sprites[SpriteIndex]);
    else if (AlienType == 7)
    {
        renderSprite(camera, viewPort, Sprites[SpriteIndex]);
        if (PendingSummon)
            renderSprite(camera, viewPort, Sprites[72]);
    }
    else if (AlienType == 8)
    {
        if (Position.Z > 0)
        {
            renderSprite(camera, viewPort, Sprites[4]);
            renderSprite(ox::core::CPosition2d<float>(0, Position.Z) + camera,
                viewPort, Sprites[SpriteIndex]);
        }
        else
            renderSprite(camera, viewPort, Sprites[SpriteIndex]);
    }
    else
        renderSprite(camera, viewPort, Sprites[0]);
    ox::core::CPosition2d<float> position;
    position.X = Position.X - camera.X;
    position.Y = Position.Y - camera.Y;
    if (viewPort.UpperLeftCorner.X > position.X || position.X > viewPort.LowerRightCorner.X ||
        viewPort.UpperLeftCorner.Y > position.Y || position.Y > viewPort.LowerRightCorner.Y)
    {
        position.X = ox::core::clamp(position.X, viewPort.UpperLeftCorner.X + 3.0f, viewPort.LowerRightCorner.X - 3.0f);
        position.Y = ox::core::clamp(position.Y, viewPort.UpperLeftCorner.Y + 3.0f, viewPort.LowerRightCorner.Y - 3.0f);
    }
    else
    {
        float maximumHealth = 100.0f;
        switch (AlienType)
        {
        case 1: maximumHealth = 200.0f; break;
        case 2: maximumHealth = 0; break;
        case 3: maximumHealth = 105.0f; break;
        case 4: maximumHealth = 60.0f; break;
        case 5: maximumHealth = 150.0f; break;
        case 6: maximumHealth = 100.0f; break;
        case 7: maximumHealth = 100.0f; break;
        case 8: maximumHealth = 155.0f; break;
        }
        if (Chant)
        {
            float value = (unsigned char)Chant;
            maximumHealth += (value * value + value) * 50.0f;
        }
        if (Health < maximumHealth && maximumHealth > 0)
            renderSelfProgress(ox::core::CPosition2d<float>(0, Position.Z) + camera,
                viewPort, Health / maximumHealth, ox::video::SColor(0xff40c020));
    }
    if (ShieldSprite && !ShieldSprite->hasFlag(1))
    {
        position.X = Position.X - camera.X + viewPort.UpperLeftCorner.X;
        position.Y = Position.Y - camera.Y + viewPort.UpperLeftCorner.Y;
        ShieldSprite->drawScaled(position, 1.0f, Color);
    }
    if (Chant && m_nextChantTime < 0 && gp_alienChantFont)
    {
        ox::core::CString<wchar_t> key(L"alienchants:chant");
        key.append(m_currentChantLine);
        ox::core::CString<wchar_t> text = settings::gp_systemConfig->getLocalizedText(key.c_str());
        ox::core::CDimension2d<int> size = gp_alienChantFont->getDimension(text.c_str());
        position.X = Position.X - camera.X + viewPort.UpperLeftCorner.X;
        position.Y = Position.Y - camera.Y + viewPort.UpperLeftCorner.Y;
        ox::core::CPosition2d<int> corner((int)position.X - size.Width / 2, (int)position.Y - 30);
        gp_alienChantFont->draw(text.c_str(), ox::core::CRect<int>(corner, size),
            ox::video::SColor(0xffffffff), ox::gui::EFHA_LEFT, ox::gui::EFVA_TOP, 0);
    }
}

void CAlienEntity::writeEntityData(ox::io::IWriteFile* file)
{
    ox::io::CHelpIO::writeInt(file, Target.Id);
    ox::io::CHelpIO::writeFloat(file, Health);
    ox::io::CHelpIO::writeFloat(file, Speed.X);
    ox::io::CHelpIO::writeFloat(file, Speed.Y);
    ox::io::CHelpIO::writeInt(file, AlienType);
    ox::io::CHelpIO::writeFloat(file, Angle);
    ox::io::CHelpIO::writeInt(file, SummonerCharging);
    ox::io::CHelpIO::writeInt(file, AnimationDone);
    ox::io::CHelpIO::writeInt(file, PendingSummon);
    ox::io::CHelpIO::writeFloat(file, StateTimer);
    ox::io::CHelpIO::writeFloat(file, StateAngle);
    ox::io::CHelpIO::writeFloat(file, TargetAngle);
    ox::io::CHelpIO::writeInt(file, SpriteIndex);
    ox::io::CHelpIO::writeInt(file, HoggerAttached);
    ox::io::CHelpIO::writeInt(file, EatCount);
    ox::io::CHelpIO::writeInt(file, MinerLanding);
    ox::io::CHelpIO::writeFloat(file, Position.Z);
    ox::io::CHelpIO::writeInt(file, Invisible);
    ox::io::CHelpIO::writeFloat(file, JumpSpeed.X);
    ox::io::CHelpIO::writeFloat(file, JumpSpeed.Y);
    Sprites[SpriteIndex]->write(file);
    ox::io::CHelpIO::writeByte(file, Chant);
}

void CAlienEntity::readEntityData(ox::io::IReadFile* file, int version)
{
    Target.Id = ox::io::CHelpIO::readInt(file);
    Health = ox::io::CHelpIO::readFloat(file);
    Speed.X = ox::io::CHelpIO::readFloat(file);
    Speed.Y = ox::io::CHelpIO::readFloat(file);
    AlienType = ox::io::CHelpIO::readInt(file);
    if (version >= 3)
    {
        Angle = ox::io::CHelpIO::readFloat(file);
        SummonerCharging = ox::io::CHelpIO::readInt(file) != 0;
        AnimationDone = ox::io::CHelpIO::readInt(file) != 0;
        PendingSummon = ox::io::CHelpIO::readInt(file) != 0;
        StateTimer = ox::io::CHelpIO::readFloat(file);
        StateAngle = ox::io::CHelpIO::readFloat(file);
    }
    if (version >= 4)
    {
        TargetAngle = ox::io::CHelpIO::readFloat(file);
        SpriteIndex = ox::io::CHelpIO::readInt(file);
        HoggerAttached = ox::io::CHelpIO::readInt(file);
        EatCount = ox::io::CHelpIO::readInt(file);
        MinerLanding = ox::io::CHelpIO::readInt(file) != 0;
    }
    switch (AlienType)
    {
    case 1: replaceWithJammerSprite(); break;
    case 2: replaceWithTinySprites(); break;
    case 3: replaceWithSummonerSprites(); break;
    case 4: replaceWithLookerSprites(); break;
    case 5: replaceWithMinerSprites(); break;
    case 6: replaceWithStealerSprites(); break;
    case 7: replaceWithMagnetoSprites(); break;
    case 8: replaceWithMegaSprites(); break;
    }
    if (version >= 6)
    {
        Position.Z = ox::io::CHelpIO::readFloat(file);
        Invisible = ox::io::CHelpIO::readInt(file) != 0;
        JumpSpeed.X = ox::io::CHelpIO::readFloat(file);
        JumpSpeed.Y = ox::io::CHelpIO::readFloat(file);
        Sprites[SpriteIndex]->read(file);
    }
    if (version >= 28)
        Chant = ox::io::CHelpIO::readByte(file);
    SpawnCooldown = 0;
}

CAlienLuaInfo::CAlienLuaInfo(lua_State* L) : Entity(0) {}

CAlienLuaInfo::~CAlienLuaInfo() {}

int CAlienLuaInfo::getId(lua_State* L)
{
    lua_pushnumber(L, Entity->getId());
    return 1;
}

int CAlienLuaInfo::getAlienType(lua_State* L)
{
    lua_pushnumber(L, Entity->AlienType + 1);
    return 1;
}

int CAlienLuaInfo::getPosition(lua_State* L)
{
    lua_pushnumber(L, Entity->getPosition().X);
    lua_pushnumber(L, Entity->getPosition().Y);
    lua_pushnumber(L, Entity->getPosition().Z);
    return 3;
}

int CAlienLuaInfo::dealDamage(lua_State* L)
{
    if (lua_gettop(L) < 5)
        return 0;
    float damage = lua_tonumber(L, 1);
    double x = lua_tonumber(L, 2);
    double y = lua_tonumber(L, 3);
    double force = lua_tonumber(L, 4);
    int weapon = lua_tointeger(L, 5);
    bool killed = Entity->dealDamage(damage, ox::core::CPosition2d<float>(x, y), force, weapon);
    lua_pushnumber(L, damage);
    lua_pushboolean(L, killed);
    return 2;
}

int CAlienLuaInfo::applyForce(lua_State* L)
{
    if (lua_gettop(L) >= 2)
    {
        double x = lua_tonumber(L, 1);
        double y = lua_tonumber(L, 2);
        Entity->Speed += ox::core::CVector2d<float>(x, y);
    }
    return 0;
}

int CAlienLuaInfo::setPosition(lua_State* L)
{
    int count = lua_gettop(L);
    if (count >= 2)
    {
        double x = lua_tonumber(L, 1);
        double y = lua_tonumber(L, 2);
        float z = 0;
        if (count >= 3)
            z = lua_tonumber(L, 3);
        Entity->setPosition(x, y, z);
    }
    return 0;
}

int CAlienLuaInfo::getDeathParticle(lua_State* L)
{
    lua_pushstring(L, CAlienEntity::getDeathParticleName(Entity->AlienType));
    return 1;
}

int CAlienLuaInfo::remove(lua_State* L)
{
    Entity->killEntity();
    return 0;
}

int CAlienLuaInfo::setTargetBuilding(lua_State* L)
{
    if (lua_gettop(L) >= 1)
    {
        int id = lua_tointeger(L, 1);
        CAlienEntity* entity = Entity;
        entity->Target.Id = id;
        entity->Target.Entity = 0;
    }
    return 0;
}


} // end namespace entity
} // end namespace harvest
