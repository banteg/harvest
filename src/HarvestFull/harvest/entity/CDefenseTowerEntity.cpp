// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include <cmath>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CDefenseTowerEntity.h"
#include "harvest/entity/CAlienEntity.h"
#include "harvest/entity/CMissileTurretEntity.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/algo/CRand.h"
#include "ox/core/CBasic.h"
#include "ox/core/CMath.h"
#include "ox/event/IEventReceiver.h"
#include "ox/io/CHelpIO.h"
#include "ox/video/IParticlePackage.h"
#include "ox/video/IParticleState.h"
#include "ox/video/ISpritePackage.h"

namespace harvest {
namespace entity {

CDefenseTowerEntity::CDefenseTowerEntity(float x, float y)
    : CBuildingEntity(g_nextEntityId++, 7, x, y), Energy(0), Shooting(false), DrawLaser(false),
      SearchCooldown(0), LaserPhase(0), Kills(0), Beam(6.0f),
      TargetPosition(0, 0, -1), BackTargetCount(1), ShootingBackTargetCount(0), HitParticle(0),
      Rotation(0), SpriteIndex(0)
{
    Sprites[0] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0000"));
    Sprites[1] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0001"));
    Sprites[2] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0002"));
    Sprites[3] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0003"));
    Sprites[4] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0004"));
    Sprites[5] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0005"));
    Sprites[6] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0006"));
    Sprites[7] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0007"));
    Sprites[8] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0008"));
    Sprites[9] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0009"));
    Sprites[10] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0010"));
    Sprites[11] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0011"));
    Sprites[12] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0012"));
    Sprites[13] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0013"));
    Sprites[14] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0014"));
    Sprites[15] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0015"));
    Sprites[16] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0016"));
    Sprites[17] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0017"));
    Sprites[18] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0018"));
    Sprites[19] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0019"));
    Sprites[20] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0020"));
    Sprites[21] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0021"));
    Sprites[22] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0022"));
    Sprites[23] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0023"));
    Sprites[24] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0024"));
    Sprites[25] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0025"));
    Sprites[26] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0026"));
    Sprites[27] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0027"));
    Sprites[28] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0028"));
    Sprites[29] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0029"));
    Sprites[30] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0030"));
    Sprites[31] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0031"));
    Sprites[32] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0032"));
    Sprites[33] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0033"));
    Sprites[34] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0034"));
    Sprites[35] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0035"));
    Sprites[36] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0036"));
    Sprites[37] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0037"));
    Sprites[38] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0038"));
    Sprites[39] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0039"));
    Sprites[40] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0040"));
    Sprites[41] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0041"));
    Sprites[42] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0042"));
    Sprites[43] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0043"));
    Sprites[44] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0044"));
    Sprites[45] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0045"));
    Sprites[46] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0046"));
    Sprites[47] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0047"));
    Sprites[48] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0048"));
    Sprites[49] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0049"));
    Sprites[50] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0050"));
    Sprites[51] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0051"));
    Sprites[52] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0052"));
    Sprites[53] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0053"));
    Sprites[54] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0054"));
    Sprites[55] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0055"));
    Sprites[56] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0056"));
    Sprites[57] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0057"));
    Sprites[58] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0058"));
    Sprites[59] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0059"));
    Sprites[60] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0060"));
    Sprites[61] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0061"));
    Sprites[62] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0062"));
    Sprites[63] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0063"));
    Sprites[64] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0064"));
    Sprites[65] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0065"));
    Sprites[66] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0066"));
    Sprites[67] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0067"));
    Sprites[68] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0068"));
    Sprites[69] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0069"));
    Sprites[70] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0070"));
    Sprites[71] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTower0071"));
    Sprites[72] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0000"));
    Sprites[73] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0001"));
    Sprites[74] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0002"));
    Sprites[75] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0003"));
    Sprites[76] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0004"));
    Sprites[77] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0005"));
    Sprites[78] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0006"));
    Sprites[79] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0007"));
    Sprites[80] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0008"));
    Sprites[81] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0009"));
    Sprites[82] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0010"));
    Sprites[83] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0011"));
    Sprites[84] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0012"));
    Sprites[85] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0013"));
    Sprites[86] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0014"));
    Sprites[87] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0015"));
    Sprites[88] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0016"));
    Sprites[89] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0017"));
    Sprites[90] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0018"));
    Sprites[91] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0019"));
    Sprites[92] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0020"));
    Sprites[93] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0021"));
    Sprites[94] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0022"));
    Sprites[95] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0023"));
    Sprites[96] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0024"));
    Sprites[97] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0025"));
    Sprites[98] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0026"));
    Sprites[99] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0027"));
    Sprites[100] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0028"));
    Sprites[101] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0029"));
    Sprites[102] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0030"));
    Sprites[103] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0031"));
    Sprites[104] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0032"));
    Sprites[105] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0033"));
    Sprites[106] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0034"));
    Sprites[107] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0035"));
    Sprites[108] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0036"));
    Sprites[109] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0037"));
    Sprites[110] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0038"));
    Sprites[111] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0039"));
    Sprites[112] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0040"));
    Sprites[113] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0041"));
    Sprites[114] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0042"));
    Sprites[115] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0043"));
    Sprites[116] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0044"));
    Sprites[117] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0045"));
    Sprites[118] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0046"));
    Sprites[119] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0047"));
    Sprites[120] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0048"));
    Sprites[121] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0049"));
    Sprites[122] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0050"));
    Sprites[123] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0051"));
    Sprites[124] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0052"));
    Sprites[125] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0053"));
    Sprites[126] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0054"));
    Sprites[127] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0055"));
    Sprites[128] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0056"));
    Sprites[129] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0057"));
    Sprites[130] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0058"));
    Sprites[131] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0059"));
    Sprites[132] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0060"));
    Sprites[133] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0061"));
    Sprites[134] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0062"));
    Sprites[135] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0063"));
    Sprites[136] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0064"));
    Sprites[137] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0065"));
    Sprites[138] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0066"));
    Sprites[139] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0067"));
    Sprites[140] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0068"));
    Sprites[141] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0069"));
    Sprites[142] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0070"));
    Sprites[143] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DefenceTowerOn0071"));
    Beam.Beam = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("Laser"));
    Beam.Start = ox::core::CPosition2d<float>(Position.X, Position.Y - 16.0f);
    Beam.StartSprite = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("LaserGlow"));
    Beam.EndSprite = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("LaserSprite"));
}

CDefenseTowerEntity::~CDefenseTowerEntity()
{
    if (HitParticle)
        HitParticle->remove();
    for (int i = 0; i < 144; ++i)
        if (Sprites[i])
            Sprites[i]->remove();
    for (unsigned int i = 0; i < BackTargets.size(); ++i)
        delete BackTargets[i];
}

int CDefenseTowerEntity::updateLogic(float frameDelta)
{
    updateTowerLinks();
    bool showLaser = false;
    bool noHitParticle = false;
    int alpha = 162;
    if (Energy.getValue() > 0.0f)
    {
        if (ForwardTarget.Id > 0 && ForwardTarget.Entity)
        {
            TargetPosition = ForwardTarget.Entity->getPosition();
            bool aligned = rotateTowardsTarget(frameDelta);
            if (isForwardTargetShooting() && aligned)
            {
                SearchCooldown -= frameDelta;
                if (SearchCooldown <= 0)
                {
                    Energy.modifyValue(-frameDelta);
                    Shooting = true;
                    Beam.Width = ShootingBackTargetCount + 4.0f;
                    noHitParticle = true;
                    alpha = 255;
                }
            }
            else
            {
                Shooting = false;
                Beam.Width = 1.5f;
                SearchCooldown = 0.15f;
                showLaser = aligned;
                noHitParticle = true;
                alpha = 255;
            }
            Target.Id = -1;
            Target.Entity = 0;
        }
        else
        {
            if (Target.Id > 0)
            {
                gp_entityManager->updateReference(Target, 1, true);
                if (!Target.Entity)
                    stopShootingAndClearTarget();
                else
                {
                    TargetPosition = Target.Entity->getPosition();
                    Shooting = rotateTowardsTarget(frameDelta);
                }
            }
            else if (Shooting)
                stopShootingAndClearTarget();
            if (Target.Entity)
            {
                CAlienEntity* alien = (CAlienEntity*)Target.Entity;
                if (alien->getAlienType() == 8 && alien->getPosition().Z >= 10.0f)
                    stopShootingAndClearTarget();
                else
                {
                    float dx = Position.X - Target.Entity->getPosition().X;
                    float dy = Position.Y - Target.Entity->getPosition().Y;
                    float range = getTotalRange();
                    float distance = dx * dx + dy * dy;
                    if (distance > range * range || distance < 900.0f)
                        stopShootingAndClearTarget();
                }
            }
            if (Shooting && Target.Entity)
            {
                CAlienEntity* alien = (CAlienEntity*)Target.Entity;
                Energy.modifyValue(-frameDelta);
                Beam.Width = ShootingBackTargetCount + 4.0f;
                float damage = getTotalDamage(true) * frameDelta;
                bool killed = alien->dealDamage(damage,
                    ox::core::CPosition2d<float>(Position.X, Position.Y), 0.8f, 0);
                TargetHeight = 3.0f;
                if (damage <= 0.0f && alien->getAlienType() == 4)
                {
                    ShieldHitTime += frameDelta;
                    if (ShieldHitTime >= 0.5f)
                        stopShootingAndClearTarget();
                    else
                    {
                        TargetPosition = Target.Entity->getPosition();
                        ox::core::CVector2d<float> direction(Position.X - TargetPosition.X,
                            Position.Y - TargetPosition.Y);
                        direction.normalize();
                        TargetPosition.X += direction.X * 25.0f;
                        TargetPosition.Y += direction.Y * 20.0f;
                        noHitParticle = true;
                    }
                }
                else
                {
                    TargetPosition = Target.Entity->getPosition();
                    if (alien->getAlienType() == 5)
                        TargetHeight = 23.0f;
                }
                if (0.0f >= Energy.getValue() || killed)
                    stopShootingAndClearTarget();
                if (killed)
                {
                    ++Kills;
                    if (Kills == 1000)
                    {
                        ox::event::SEvent event;
                        event.EventType = ox::event::EET_USER_EVENT;
                        event.UserEvent.UserData1 = 21;
                        event.UserEvent.UserData2 = 14;
                        event.UserEvent.UserData3 = 0;
                        event.UserEvent.UserPointer = 0;
                        ox::event::gp_subscriberList->OnEvent(event);
                    }
                }
                alpha = 255;
            }
            else if (!Target.Entity)
            {
                SearchCooldown -= frameDelta;
                if (SearchCooldown <= 0)
                {
                    float range = getTotalRange();
                    ox::core::CVector3d<float> nearest(0, 0, -1);
                    Target.Entity = CMissileTurretEntity::findPriorityAlien(Position, BackTargetCount > 1,
                        range * range, 900.0f, &nearest);
                    if (Target.Entity)
                    {
                        Target.Id = Target.Entity->getId();
                        Target.UpdateCounter = gp_entityManager->getUpdateCounter();
                        TargetPosition = Target.Entity->getPosition();
                        ShieldHitTime = 0;
                        TargetHeight = 3.0f;
                    }
                    else
                    {
                        SearchCooldown = (ox::algo::CRand::rand() % 1000) * 0.001f + 1.0f;
                        if (nearest.Z >= 0)
                            TargetPosition = nearest;
                    }
                }
                else if (TargetPosition.Z >= 0)
                    rotateTowardsTarget(frameDelta);
            }
        }
    }
    else
        stopShootingAndClearTarget();
    Color = Energy.getValue() > 0.0f ? ox::video::SColor(0xffffffff) : ox::video::SColor(0xff80a280);
    if (!Shooting)
    {
        if (SpriteIndex >= 72)
            SpriteIndex -= 72;
    }
    else if (SpriteIndex < 72)
        SpriteIndex += 72;
    if (Shooting || showLaser)
    {
        DrawLaser = true;
        LaserPhase += 4.0f * frameDelta;
        if (ForwardTarget.Id > 0)
        {
            Beam.End.X = TargetPosition.X;
            Beam.End.Y = TargetPosition.Y - 16.0f;
        }
        else
        {
            Beam.End.X = TargetPosition.X;
            Beam.End.Y = TargetPosition.Y;
        }
        int laserAlpha = int(sin(LaserPhase) * 60.0) + alpha;
        Beam.Color = ox::video::SColor(ox::core::min_(laserAlpha, 255), 255, 255, 255);
        if (noHitParticle)
        {
            if (HitParticle)
            {
                HitParticle->remove();
                HitParticle = 0;
            }
        }
        else
        {
            if (!HitParticle)
                HitParticle = gp_particlePackage->addNewParticleState(ox::core::CString<char>("LaserHit"));
            if (HitParticle)
            {
                ox::core::CVector3d<float> position(Beam.End.X, Beam.End.Y + 3.0f, TargetHeight);
                HitParticle->update(frameDelta, position);
            }
        }
        Beam.End.Y -= TargetHeight;
    }
    else
    {
        DrawLaser = false;
        if (HitParticle)
        {
            HitParticle->remove();
            HitParticle = 0;
        }
    }
    return 0;
}

void CDefenseTowerEntity::updateTowerLinks()
{
    if (ForwardTarget.Id > 0)
    {
        gp_entityManager->updateReference(ForwardTarget, 0, true);
        if (!ForwardTarget.Entity || ForwardTarget.Entity->getEntityType() != 7)
        {
            ForwardTarget.Entity = 0;
            ForwardTarget.Id = -1;
        }
        else
            TargetHeight = 16.0f;
    }
    bool removed = false;
    for (unsigned int i = 0; i < BackTargets.size(); ++i)
    {
        gp_entityManager->updateReference(*BackTargets[i], 0, true);
        if (!BackTargets[i]->Entity)
            removed = true;
    }
    if (removed)
    {
        std::vector<ox::entity::SEntityReference*>::iterator it = BackTargets.begin();
        while (it != BackTargets.end())
        {
            if (!(*it)->Entity)
            {
                delete *it;
                BackTargets.erase(it);
                it = BackTargets.begin();
            }
            else
                ++it;
        }
    }
    updateBackTargetCount();
    updateForwardTargetShooting();
}

bool CDefenseTowerEntity::rotateTowardsTarget(float frameDelta)
{
    float angle = ox::core::CMath::getAngleIY(ox::core::CPosition2d<float>(Position.X, Position.Y),
        ox::core::CPosition2d<float>(TargetPosition.X, TargetPosition.Y));
    if (angle < 0)
        angle += 6.28318548f;
    else if (angle > 6.28318548f)
        angle -= 6.28318548f;
    if (frameDelta < 0.001f)
        return ox::core::abs_(angle - Rotation) < 0.01f;
    bool finished = false;
    if (ox::core::CMath::clockWiseClosestIY(Rotation, angle))
    {
        Rotation += frameDelta * 3.0f;
        if (!ox::core::CMath::clockWiseClosestIY(Rotation, angle))
        {
            Rotation = angle;
            finished = true;
        }
    }
    else
    {
        Rotation += frameDelta * -3.0f;
        if (ox::core::CMath::clockWiseClosestIY(Rotation, angle))
        {
            Rotation = angle;
            finished = true;
        }
    }
    while (Rotation < 0)
        Rotation += 6.28318548f;
    while (Rotation > 6.28318548f)
        Rotation -= 6.28318548f;
    int oldIndex = SpriteIndex;
    SpriteIndex = 0;
    if (Rotation > 0.0436332338f && Rotation < 6.23955202f)
        SpriteIndex = 71 - int((Rotation - 0.0436332338f) / 0.0872664675f);
    if (oldIndex != SpriteIndex)
    {
        Beam.Start.X = Position.X + cos(Rotation) * 32.0;
        Beam.Start.Y = Position.Y + sin(Rotation) * 22.0 - 25.0;
    }
    return finished;
}

bool CDefenseTowerEntity::isForwardTargetShooting()
{
    return ForwardTargetShooting;
}

int CDefenseTowerEntity::getBackTargetCount(bool shooting)
{
    return shooting ? ShootingBackTargetCount : BackTargetCount;
}

float CDefenseTowerEntity::getTotalRange()
{
    if (!BackTargets.empty())
    {
        float count = BackTargetCount - 1;
        if (count >= 1 && count < 14)
            return (21.0f - count + 20.0f) * 0.5f * count + 200.0f;
        if (count >= 14)
            return (count - 13.0f) * 7.0f + 382.0f;
    }
    return 200.0f;
}

float CDefenseTowerEntity::getTotalDamage(bool shooting)
{
    if (shooting && !Shooting)
        return 0;
    if (BackTargets.empty())
        return 11.0f;
    return (getBackTargetCount(shooting) - 1) * 5.5f + 11.0f;
}

void CDefenseTowerEntity::updateBackTargetCount()
{
    if (Energy.getValue() <= 0.0f)
    {
        BackTargetCount = 0;
        ShootingBackTargetCount = 0;
    }
    else
    {
        BackTargetCount = 1;
        ShootingBackTargetCount = Shooting;
        for (unsigned int i = 0; i < BackTargets.size(); ++i)
        {
            BackTargetCount += ((CDefenseTowerEntity*)BackTargets[i]->Entity)->getBackTargetCount(false);
            ShootingBackTargetCount += ((CDefenseTowerEntity*)BackTargets[i]->Entity)->getBackTargetCount(true);
        }
        if (BackTargetCount >= 101)
        {
            ox::event::SEvent event;
            event.EventType = ox::event::EET_USER_EVENT;
            event.UserEvent.UserData1 = 21;
            event.UserEvent.UserData2 = 21;
            event.UserEvent.UserData3 = 0;
            event.UserEvent.UserPointer = 0;
            ox::event::gp_subscriberList->OnEvent(event);
        }
    }
}

void CDefenseTowerEntity::updateForwardTargetShooting()
{
    if (Energy.getValue() <= 0.0f)
        ForwardTargetShooting = false;
    else if (ForwardTarget.Entity)
        ForwardTargetShooting = ((CDefenseTowerEntity*)ForwardTarget.Entity)->Shooting;
    else
        ForwardTargetShooting = Shooting;
}

void CDefenseTowerEntity::updateSprite(float frameDelta)
{
    if (Beam.Beam)
        Beam.Beam->update(frameDelta);
    if (Sprites[SpriteIndex])
        Sprites[SpriteIndex]->update(frameDelta);
}

void CDefenseTowerEntity::handleRightClickAction(const ox::core::CPosition2d<float>& position)
{
    CEntity* entity = gp_entityManager->findClickableEntity(position);
    if (entity && entity->getEntityType() == 7)
    {
        if (entity == this)
        {
            if (ForwardTarget.Entity)
                ((CDefenseTowerEntity*)ForwardTarget.Entity)->notifyRemoveBackTarget(this);
            ForwardTarget.Id = -1;
            ForwardTarget.Entity = 0;
        }
        else
            handleSelectionDraggedToEntity(entity);
    }
}

void CDefenseTowerEntity::notifyRemoveBackTarget(CDefenseTowerEntity* tower)
{
    for (std::vector<ox::entity::SEntityReference*>::iterator it = BackTargets.begin(); it != BackTargets.end(); ++it)
    {
        if ((*it)->Entity == tower)
        {
            delete *it;
            BackTargets.erase(it);
            --BackTargetCount;
            break;
        }
    }
}

void CDefenseTowerEntity::handleDoubleClickSelection()
{
    if (ForwardTarget.Entity)
    {
        CDefenseTowerEntity* tower = (CDefenseTowerEntity*)ForwardTarget.Entity;
        ForwardTarget.Id = -1;
        ForwardTarget.Entity = 0;
        tower->notifyRemoveBackTarget(this);
        tower->handleDoubleClickSelection();
    }
}

void CDefenseTowerEntity::handleSelectionDraggedToNothing()
{
    if (ForwardTarget.Entity)
    {
        CDefenseTowerEntity* tower = (CDefenseTowerEntity*)ForwardTarget.Entity;
        ForwardTarget.Id = -1;
        ForwardTarget.Entity = 0;
        tower->notifyRemoveBackTarget(this);
    }
}

bool CDefenseTowerEntity::handleSelectionDraggedToEntity(CEntity* entity)
{
    if (entity->getEntityType() == 7 && entity != this)
    {
        float dx = Position.X - entity->getPosition().X;
        float dy = Position.Y - entity->getPosition().Y;
        CDefenseTowerEntity* tower = (CDefenseTowerEntity*)entity;
        if (dx * dx + dy * dy <= 40000.0f && !tower->isForwardingToThis(this))
        {
            if (ForwardTarget.Entity)
                ((CDefenseTowerEntity*)ForwardTarget.Entity)->notifyRemoveBackTarget(this);
            ForwardTarget.Entity = entity;
            ForwardTarget.Id = entity->getId();
            ForwardTarget.UpdateCounter = gp_entityManager->getUpdateCounter();
            ((CDefenseTowerEntity*)ForwardTarget.Entity)->notifyNewBackTarget(this);
            return true;
        }
    }
    return false;
}

bool CDefenseTowerEntity::isForwardingToThis(CDefenseTowerEntity* tower)
{
    if (this == tower)
        return true;
    if (ForwardTarget.Entity)
        return ForwardTarget.Entity == tower
            || ((CDefenseTowerEntity*)ForwardTarget.Entity)->isForwardingToThis(tower);
    return false;
}

void CDefenseTowerEntity::notifyNewBackTarget(CDefenseTowerEntity* tower)
{
    ox::entity::SEntityReference* reference = new ox::entity::SEntityReference;
    reference->Entity = tower;
    reference->Id = tower->getId();
    reference->UpdateCounter = gp_entityManager->getUpdateCounter();
    BackTargets.push_back(reference);
}

bool CDefenseTowerEntity::isLinked()
{
    return ForwardTarget.Entity != 0;
}

bool CDefenseTowerEntity::isLinkedTo(CDefenseTowerEntity* tower)
{
    return ForwardTarget.Entity == tower;
}

void CDefenseTowerEntity::makeEndLaser(CDefenseTowerEntity* tower)
{
    CDefenseTowerEntity* next = (CDefenseTowerEntity*)ForwardTarget.Entity;
    if (tower)
        handleSelectionDraggedToEntity(tower);
    else
        handleSelectionDraggedToNothing();
    if (next)
        next->makeEndLaser(this);
}

int CDefenseTowerEntity::getNumBackTargets()
{
    return BackTargetCount - 1;
}

ox::core::CString<wchar_t> CDefenseTowerEntity::getInfoString()
{
    ox::core::CString<wchar_t> result;
    if (Energy.getValue() <= 0)
        result = settings::gp_systemConfig->getLocalizedText(L"entity:defenseOutOfEnergy");
    else
    {
        float range = getTotalRange();
        int shootingDamage = int(getTotalDamage(true) / 11.0f * 100.0f);
        int totalDamage = int(getTotalDamage(false) / 11.0f * 100.0f);
        result = settings::gp_systemConfig->getLocalizedText(L"entity:defenseRangeAndDamage",
            int(range / 200.0f * 100.0f), shootingDamage, totalDamage);
    }
    return ox::core::CString<wchar_t>(result);
}

ox::core::CString<wchar_t> CDefenseTowerEntity::getMiniStatString()
{
    if (Kills == 1)
        return settings::gp_systemConfig->getLocalizedText(L"entity:alienKill", Kills);
    return settings::gp_systemConfig->getLocalizedText(L"entity:alienKills", Kills);
}

ox::core::CString<wchar_t> CDefenseTowerEntity::getOperatorString()
{
    return ox::core::CString<wchar_t>(L"");
}

int CDefenseTowerEntity::onSpark(CSparkEntity* spark)
{
    if (Energy.getValue() <= 17.25f)
    {
        Energy.setValue(ox::core::min_(Energy.getValue() + 2.75f, 20.0f));
        return 0;
    }
    return -1;
}

bool CDefenseTowerEntity::wantsSpark()
{
    return Energy.getValue() + 2.75f <= 20.0f;
}

bool CDefenseTowerEntity::addToRenderList(const ox::core::CRect<float>& visibleArea)
{
    if (visibleArea.isPointInside(ox::core::CPosition2d<float>(Position.X, Position.Y)))
        return true;
    return visibleArea.isPointInside(ox::core::CPosition2d<float>(Beam.End.X, Beam.End.Y));
}

void CDefenseTowerEntity::render(const ox::core::CPosition2d<float>& camera,
    const ox::core::CRect<int>& viewPort)
{
    if (DrawLaser)
    {
        if (!Shooting || ForwardTarget.Id > 0)
            Beam.EndScale = Beam.Width / 5.0f * 0.25f;
        else
            Beam.EndScale = Beam.Width / 5.0f;
        gp_entityManager->insertTopLevelEnergyBeam(&Beam);
    }
    renderSpriteFixed(camera, viewPort, Sprites[SpriteIndex]);
    if (Energy.getValue() < 17.25f)
    {
        float progress = Energy.getValue() / 20.0f;
        renderSelfProgress(camera, viewPort, progress, ENERGY_PROGRESS_COLOR);
    }
}

ox::video::ISpriteAnimationState* CDefenseTowerEntity::getCurrentDisplaySprite()
{
    return Sprites[SpriteIndex];
}

void CDefenseTowerEntity::writeEntityData(ox::io::IWriteFile* file)
{
    ox::io::CHelpIO::writeInt(file, Target.Id);
    ox::io::CHelpIO::writeFloat(file, Energy.getValue());
    ox::io::CHelpIO::writeInt(file, Kills);
    ox::io::CHelpIO::writeInt(file, Shooting);
    ox::io::CHelpIO::writeFloat(file, ShieldHitTime);
    ox::io::CHelpIO::writeInt(file, ForwardTarget.Id);
    ox::io::CHelpIO::writeInt(file, BackTargets.size());
    for (unsigned int i = 0; i < BackTargets.size(); ++i)
        ox::io::CHelpIO::writeInt(file, BackTargets[i]->Id);
    ox::io::CHelpIO::writeInt(file, BackTargetCount);
    ox::io::CHelpIO::writeInt(file, ShootingBackTargetCount);
    ox::io::CHelpIO::writeInt(file, ForwardTargetShooting);
    ox::io::CHelpIO::writeInt(file, SpriteIndex);
    ox::io::CHelpIO::writeFloat(file, Rotation);
}

void CDefenseTowerEntity::readEntityData(ox::io::IReadFile* file, int version)
{
    Target.Id = ox::io::CHelpIO::readInt(file);
    Energy.setValue(ox::io::CHelpIO::readFloat(file));
    if (version >= 2)
        Kills = ox::io::CHelpIO::readInt(file);
    if (version >= 6)
    {
        Shooting = ox::io::CHelpIO::readInt(file);
        ShieldHitTime = ox::io::CHelpIO::readFloat(file);
        ForwardTarget.Id = ox::io::CHelpIO::readInt(file);
        int count = ox::io::CHelpIO::readInt(file);
        for (int i = 0; i < count; ++i)
        {
            ox::entity::SEntityReference* reference = new ox::entity::SEntityReference;
            reference->Id = ox::io::CHelpIO::readInt(file);
            BackTargets.push_back(reference);
        }
        BackTargetCount = ox::io::CHelpIO::readInt(file);
        ShootingBackTargetCount = ox::io::CHelpIO::readInt(file);
        ForwardTargetShooting = ox::io::CHelpIO::readInt(file);
    }
    if (version >= 8)
    {
        SpriteIndex = ox::io::CHelpIO::readInt(file);
        Rotation = ox::io::CHelpIO::readFloat(file);
    }
}

} // end namespace entity
} // end namespace harvest
