// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
#include <iostream>
#include <vector>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CDropshipEntity.h"
#include "harvest/entity/CAlienEntity.h"
#include "harvest/entity/CMissileTurretEntity.h"
#include "ox/algo/CRand.h"
#include "ox/core/CBasic.h"
#include "ox/core/CMath.h"
#include "ox/event/IEventReceiver.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"
namespace harvest {
namespace entity {
CDropshipEntity::CDropshipEntity(float x, float y, bool takeoff)
    : CEntity(g_nextEntityId++, 17, x, y), SpriteIndex(0), Angle(0), Speed(0), SpriteAngle(0),
      State(0), StateTime(0), SoundPitch(.75f), TargetPosition(500, 500, 0), LandingSteps(3),
      BulletCooldown(0), MissileCooldown(2), SalvoCooldown(0), SalvoCount(1), LaughCooldown(0),
      LaughIndex(0), Beam(4.0f), UnrecoveredFlag(false)
{
    for (int i = 0; i < 77; ++i)
        Sprites[i] = 0;
    Position.Z = 100.0f;
    loadSprite(takeoff);
    Beam.Beam = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("EnergyRedirect"));
    if (takeoff)
    {
        Position.Z = 7.0f;
        State = 5;
        SpriteIndex = 74;
        Angle = 5.49778748f;
    }
}

void CDropshipEntity::loadSprite(bool takeoff)
{
    if (gp_videoDriver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", false))
    {
        if (!takeoff)
        {
            Sprites[0] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0000"));
            Sprites[1] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0001"));
            Sprites[2] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0002"));
            Sprites[3] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0003"));
            Sprites[4] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0004"));
            Sprites[5] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0005"));
            Sprites[6] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0006"));
            Sprites[7] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0007"));
            Sprites[8] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0008"));
            Sprites[9] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0009"));
            Sprites[10] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0010"));
            Sprites[11] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0011"));
            Sprites[12] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0012"));
            Sprites[13] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0013"));
            Sprites[14] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0014"));
            Sprites[15] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0015"));
            Sprites[16] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0016"));
            Sprites[17] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0017"));
            Sprites[18] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0018"));
            Sprites[19] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0019"));
            Sprites[20] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0020"));
            Sprites[21] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0021"));
            Sprites[22] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0022"));
            Sprites[23] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0023"));
            Sprites[24] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0024"));
            Sprites[25] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0025"));
            Sprites[26] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0026"));
            Sprites[27] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0027"));
            Sprites[28] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0028"));
            Sprites[29] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0029"));
            Sprites[30] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0030"));
            Sprites[31] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0031"));
            Sprites[32] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0032"));
            Sprites[33] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0033"));
            Sprites[34] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0034"));
            Sprites[35] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0035"));
            Sprites[36] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0036"));
            Sprites[37] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0037"));
            Sprites[38] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0038"));
            Sprites[39] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0039"));
            Sprites[40] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0040"));
            Sprites[41] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0041"));
            Sprites[42] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0042"));
            Sprites[43] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0043"));
            Sprites[44] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0044"));
            Sprites[45] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0045"));
            Sprites[46] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0046"));
            Sprites[47] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0047"));
            Sprites[48] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0048"));
            Sprites[49] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0049"));
            Sprites[50] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0050"));
            Sprites[51] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0051"));
            Sprites[52] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0052"));
            Sprites[53] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0053"));
            Sprites[54] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0054"));
            Sprites[55] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0055"));
            Sprites[56] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0056"));
            Sprites[57] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0057"));
            Sprites[58] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0058"));
            Sprites[59] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0059"));
            Sprites[60] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0060"));
            Sprites[61] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0061"));
            Sprites[62] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0062"));
            Sprites[63] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0063"));
            Sprites[64] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0064"));
            Sprites[65] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0065"));
            Sprites[66] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0066"));
            Sprites[67] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0067"));
            Sprites[68] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0068"));
            Sprites[69] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0069"));
            Sprites[70] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0070"));
            Sprites[71] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFly0071"));
        }
        Sprites[72] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFlyLanding"));
        Sprites[73] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFlyLandingFinal"));
        Sprites[74] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFlyTakeOff"));
        Sprites[75] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFlyTakeOffFinal"));
        Sprites[76] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DragonFlyAccelerating"));
    }
}

CDropshipEntity::~CDropshipEntity()
{
    for (int i = 0; i < 77; ++i)
        if (Sprites[i])
            Sprites[i]->remove();
    if (gp_audioDriver)
        gp_audioDriver->stopLoopSound("DropShipEngine.ogg");
}

int CDropshipEntity::updateLogic(float frameDelta)
{
    if (LaughCooldown > 0 && State == 1)
    {
        LaughCooldown -= frameDelta;
        if (LaughCooldown <= 0)
        {
            ++LaughIndex;
            switch (LaughIndex % 3)
            {
            case 0: gp_audioDriver->playSound("Laugh1.ogg", 1.0f, 0.0f, 1.0f); break;
            case 1: gp_audioDriver->playSound("Laugh2.ogg", 1.0f, 0.0f, 1.0f); break;
            case 2: gp_audioDriver->playSound("Laugh3.ogg", 1.0f, 0.0f, 1.0f); break;
            }
        }
    }
    else
        LaughCooldown -= frameDelta;
    if (State == 0)
    {
        float distance = ox::core::CMath::getEstimateDistance(ox::core::CPosition2d<float>(Position.X, Position.Y),
            ox::core::CPosition2d<float>(TargetPosition.X, TargetPosition.Y));
        ox::core::CVector2d<float> direction(TargetPosition.X - Position.X, TargetPosition.Y - Position.Y);
        ox::core::CVector2d<float> forward(cosf(Angle), sinf(Angle));
        direction.normalize();
        if ((distance > 424.413147f || direction.X * forward.X + direction.Y * forward.Y > 0) &&
            turnTowardsTarget(frameDelta))
            Speed = ox::core::min_(Speed + 30.0f * frameDelta, 300.0f);
        else
            Speed = ox::core::max_(Speed + -40.0f * frameDelta, 160.0f);
        SpriteIndex = -1;
        SoundPitch = 1.0f;
    }
    else if (State == 1)
    {
        if (!Target.Entity)
            locateNewAlienTarget();
        gp_entityManager->updateReference(Target, 1, false);
        if (Target.Entity)
            TargetPosition = Target.Entity->getPosition();
        float distance = ox::core::CMath::getEstimateDistance(ox::core::CPosition2d<float>(Position.X, Position.Y),
            ox::core::CPosition2d<float>(TargetPosition.X, TargetPosition.Y));
        ox::core::CVector2d<float> direction(TargetPosition.X - Position.X, TargetPosition.Y - Position.Y);
        ox::core::CVector2d<float> forward(cosf(Angle), sinf(Angle));
        direction.normalize();
        float dot = direction.X * forward.X + direction.Y * forward.Y;
        bool aligned = false;
        if (distance > 424.413147f || dot > 0)
            aligned = turnTowardsTarget(frameDelta);
        if (MissileCooldown <= 0 && Target.Entity)
        {
            SalvoCooldown -= frameDelta;
            if (SalvoCooldown <= 0)
            {
                SalvoCooldown = .3f;
                --SalvoCount;
                if (SalvoCount <= 0)
                    MissileCooldown = 2.0f;
                float x = Target.Entity->getPosition().X - Position.X;
                float y = Target.Entity->getPosition().Y - Position.Y;
                if (x * x + y * y < 250000.0f && dot > .9f)
                {
                    ox::entity::COxEntity* target = Target.Entity;
                    double cs = cos((double)Angle);
                    double sn = sin((double)Angle);
                    float missileX = Position.X + cs * 15.0;
                    float missileY = Position.Y + sn * 10.0;
                    float sideX = cos((double)(Angle + 1.57079637f)) * 28.0;
                    float sideY = sin((double)(Angle + 1.57079637f)) * 24.0;
                    ox::core::CVector3d<float> particleSpeed(cs * (Speed * -2.0f), sn * (Speed * -2.0f), 0);
                    const ox::core::CVector3d<float>& targetPosition1 = target->getPosition();
                    ox::core::CPosition2d<float> aim1(targetPosition1.X, targetPosition1.Y);
                    int targetId1 = target->getId();
                    CMissileEntity* missile = new CMissileEntity(missileX, missileY, aim1, Id, 2, targetId1);
                    float x1 = missileX + sideX;
                    float y1 = missileY + sideY;
                    missile->setPosition(x1, y1, Position.Z - 5.0f);
                    missile->setSpeed(cos((double)Angle) * 300.0, sin((double)Angle) * 300.0, -100.0f);
                    missile->disableRetargeting();
                    gp_entityManager->appendEntity(missile, 3);
                    CParticleEntity* particle1 = new CParticleEntity(x1, y1, Position.Z - 5.0f,
                        &particleSpeed, "DropShipMissileAway");
                    gp_entityManager->appendEntity(particle1, 4);
                    const ox::core::CVector3d<float>& targetPosition2 = target->getPosition();
                    ox::core::CPosition2d<float> aim2(targetPosition2.X, targetPosition2.Y);
                    int targetId2 = target->getId();
                    missile = new CMissileEntity(missileX, missileY, aim2, Id, 2, targetId2);
                    float x2 = missileX - sideX;
                    float y2 = missileY - sideY;
                    missile->setPosition(x2, y2, Position.Z - 5.0f);
                    missile->setSpeed(cos((double)Angle) * 300.0, sin((double)Angle) * 300.0, -100.0f);
                    missile->disableRetargeting();
                    gp_entityManager->appendEntity(missile, 3);
                    CParticleEntity* particle2 = new CParticleEntity(x2, y2, Position.Z - 5.0f,
                        &particleSpeed, "DropShipMissileAway");
                    gp_entityManager->appendEntity(particle2, 4);
                    Target.Entity = 0;
                    Target.Id = -1;
                }
                else
                {
                    locateNewAlienTarget();
                    MissileCooldown = 1.0f;
                }
            }
        }
        else
        {
            MissileCooldown -= frameDelta;
            SalvoCount = 1;
        }
        gp_entityManager->updateReference(BulletTarget, 1, false);
        if (BulletCooldown <= 0)
        {
            BulletCooldown += .1f;
            if (!BulletTarget.Entity)
                locateNewAlienBulletTarget();
            else
            {
                float x = Position.X - BulletTarget.Entity->getPosition().X;
                float y = Position.Y - BulletTarget.Entity->getPosition().Y;
                if (x * x + y * y <= 62500.0f)
                {
                    float angle = (ox::algo::CRand::rand() % 1000) * .002f * 3.14159274f;
                    float radius = (ox::algo::CRand::rand() % 1000) * .001f * 25.0f;
                    ox::core::CVector3d<float> start = Position + ox::core::CVector3d<float>(0, 0, -5.0f);
                    gp_entityManager->appendEntity(new CDropshipBulletEntity(20.0f, start,
                        BulletTarget.Entity->getPosition() + ox::core::CVector3d<float>(sin((double)angle) * radius, 0, 0)), 3);
                    gp_entityManager->appendEntity(new CParticleEntity(start.X, start.Y, start.Z, 0, "DropShipGun"), 4);
                    if (LaughCooldown <= -5.0f)
                        LaughCooldown = (ox::algo::CRand::rand() % 1000) * .001f + 1.0f;
                }
                else
                {
                    BulletTarget.Entity = 0;
                    BulletTarget.Id = -1;
                }
            }
        }
        else
            BulletCooldown -= frameDelta;
        if (aligned)
            Speed = ox::core::min_(Speed + 30.0f * frameDelta, 300.0f);
        else
            Speed = ox::core::max_(Speed + -40.0f * frameDelta, 160.0f);
        SoundPitch = 1.0f;
    }
    else if (State == 2)
    {
        if (LandingSteps == 3 && Angle > .785398185f && Angle < 3.92699099f &&
            Position.Y < LandingSteps * 424.413147f + 848.826294f)
            TargetPosition = ox::core::CVector3d<float>(Position.X + 0.0f, LandingSteps * 424.413147f + 424.413147f + 700.0f, 0);
        else
            TargetPosition = ox::core::CVector3d<float>(-LandingSteps * 424.413147f + 500.0f, LandingSteps * 424.413147f + 700.0f, 0);
        float distance = ox::core::CMath::getEstimateDistance(ox::core::CPosition2d<float>(Position.X, Position.Y),
            ox::core::CPosition2d<float>(TargetPosition.X, TargetPosition.Y));
        ox::core::CVector2d<float> direction(TargetPosition.X - Position.X, TargetPosition.Y - Position.Y);
        ox::core::CVector2d<float> forward(cosf(Angle), sinf(Angle));
        direction.normalize();
        float dot = direction.X * forward.X + direction.Y * forward.Y;
        if (dot > .9f && distance < 424.413147f)
        {
            --LandingSteps;
            if (LandingSteps < 0)
                State = 3;
        }
        else if (dot < 0 || (Angle < 3.14159274f && Angle > 0))
            LandingSteps = 3;
        if ((LandingSteps < 3 || distance > 424.413147f || dot > .1f) &&
            turnTowardsTarget(frameDelta) && LandingSteps >= 3)
            Speed = ox::core::min_(Speed + 30.0f * frameDelta, 300.0f);
        else
            Speed = ox::core::max_(Speed + -40.0f * frameDelta, 160.0f);
        SoundPitch = 1.0f;
    }
    else if (State == 3)
    {
        SpriteIndex = 72;
        if (Position.Z < 15.0f)
            SpriteIndex = 73;
        SoundPitch = ox::core::max_(SoundPitch + -.05f * frameDelta, .75f);
        TargetPosition = ox::core::CVector3d<float>(500, 700, 0);
        turnTowardsTarget(frameDelta);
        float distance = ox::core::CMath::getEstimateDistance(ox::core::CPosition2d<float>(Position.X, Position.Y),
            ox::core::CPosition2d<float>(TargetPosition.X, TargetPosition.Y)) / 424.413147f;
        Position.Z = ox::core::max_(sqrtf(distance) * 100.0f, 7.0f);
        if (Position.Z < 7.5f)
        {
            State = 4;
            StateTime = 0;
            ox::event::SEvent event;
            event.EventType = ox::event::EET_USER_EVENT;
            event.UserEvent.UserData1 = 36;
            event.UserEvent.UserData2 = 0;
            event.UserEvent.UserData3 = 0;
            event.UserEvent.UserPointer = 0;
            ox::event::gp_subscriberList->OnEvent(event);
            gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y - 5.0f, .1f, 0, "MegaLand"), 4);
        }
        Speed = distance * 200.0f;
    }
    else if (State == 4)
    {
        SpriteIndex = 73;
        Position.Z = 7.0f;
        SoundPitch = ox::core::max_(SoundPitch + -.05f * frameDelta, .75f);
        StateTime = 0;
        Speed = 0;
    }
    else if (State == 5)
    {
        SoundPitch = ox::core::min_(SoundPitch + .1f * frameDelta, 1.25f);
        SpriteIndex = Position.Z < 75.0f ? 74 : 75;
        if (StateTime > .5f)
        {
            Position.Z = ox::core::min_(Position.Z + 7.0f * frameDelta, 100.0f);
            Speed = ox::core::min_(Speed + (frameDelta + frameDelta), 10.0f);
            if (Position.Z >= 90.0f)
            {
                State = 6;
                ox::event::SEvent event;
                event.EventType = ox::event::EET_USER_EVENT;
                event.UserEvent.UserData1 = 37;
                event.UserEvent.UserData2 = 0;
                event.UserEvent.UserData3 = 0;
                event.UserEvent.UserPointer = 0;
                ox::event::gp_subscriberList->OnEvent(event);
                if (gp_audioDriver)
                    gp_audioDriver->playSound("DropShipTakeOff.ogg", .8f, 0.0f, 1.0f);
            }
        }
    }
    else if (State == 6)
    {
        SpriteIndex = 76;
        Speed += 200.0f * frameDelta;
        Position.Z += 10.0f * frameDelta;
        if (Position.Y < -1024.0f)
            return 1;
        SoundPitch = ox::core::min_(SoundPitch + .2f * frameDelta, 2.0f);
    }
    double speed = Speed;
    Position += ox::core::CVector3d<float>(cos((double)Angle) * speed, sin((double)Angle) * speed, 0) * frameDelta;
    StateTime += frameDelta;
    if (State == 1 || State == 2 || SpriteIndex < 0)
    {
        SpriteIndex = 0;
        if (Angle > .0436332338f && Angle < 6.23955202f)
        {
            float index = (Angle - .0436332338f) / .0872664675f;
            SpriteIndex = 71 - (int)index;
            SpriteAngle = Angle + (int)(index + 1.0f) * -.0872664675f;
        }
        else
            SpriteAngle = Angle;
        SoundPitch = Speed / 200.0f;
    }
    if (gp_audioDriver)
    {
        float volume = ox::core::max_(Position.Z / 100.0f * .8f, .2f);
        float x = Position.X - g_screenCenterPos.X;
        float y = Position.Y - g_screenCenterPos.Y;
        float pan = x;
        if (pan > g_screenSizeF.Width)
            pan = g_screenSizeF.Width;
        else
            pan = ox::core::max_(-g_screenSizeF.Width, pan);
        float range = (g_screenSizeF.Width + 200.0f) * (g_screenSizeF.Height + 200.0f);
        float distance = x * x + y * y;
        if (distance < range)
            gp_audioDriver->loopSound("DropShipEngine.ogg", (1.0f - distance / range) * volume * .8f,
                pan / g_screenSizeF.Width, SoundPitch, 0.0f);
        else
            gp_audioDriver->stopLoopSound("DropShipEngine.ogg");
    }
    return 0;
}

bool CDropshipEntity::turnTowardsTarget(float frameDelta)
{
    float angle = ox::core::CMath::getAngleIY(ox::core::CPosition2d<float>(Position.X, Position.Y),
        ox::core::CPosition2d<float>(TargetPosition.X, TargetPosition.Y));
    if (angle < 0) angle += 6.28318548f;
    else if (angle > 6.28318548f) angle -= 6.28318548f;
    bool aligned = Angle == angle;
    float step = .471238941f;
    if (ox::core::CMath::clockWiseClosestIY(Angle, angle))
    {
        Angle += step * frameDelta;
        if (!ox::core::CMath::clockWiseClosestIY(Angle, angle))
        {
            Angle = angle;
            aligned = true;
        }
    }
    else
    {
        Angle += -step * frameDelta;
        if (ox::core::CMath::clockWiseClosestIY(Angle, angle))
        {
            Angle = angle;
            aligned = true;
        }
    }
    while (Angle < 0) Angle += 6.28318548f;
    while (Angle > 6.28318548f) Angle -= 6.28318548f;
    return aligned;
}

void CDropshipEntity::locateNewAlienTarget()
{
    const std::list<ox::entity::COxEntity*>& aliens = gp_entityManager->getEntityList(1);
    float bestAngle = 1000.0f;
    for (std::list<ox::entity::COxEntity*>::const_iterator it = aliens.begin(); it != aliens.end(); ++it)
    {
        if ((*it)->getEntityType() == 6)
        {
            float x = Position.X - (*it)->getPosition().X;
            float y = Position.Y - (*it)->getPosition().Y;
            float distance = x * x + y * y;
            if (distance <= 4000000.0f && distance > 180126.516f)
            {
                float angle = Angle;
                const ox::core::CVector3d<float>& position = (*it)->getPosition();
                angle -= ox::core::CMath::getAngleIY(
                    ox::core::CPosition2d<float>(Position.X, Position.Y),
                    ox::core::CPosition2d<float>(position.X, position.Y));
                if (angle > 3.14159274f)
                    angle -= 6.28318548f;
                angle = ox::core::abs_(angle);
                if (angle < bestAngle)
                {
                    Target.Entity = *it;
                    Target.Id = (*it)->getId();
                    Target.UpdateCounter = gp_entityManager->getUpdateCounter();
                    bestAngle = angle;
                }
            }
        }
    }
    if (aliens.empty())
    {
        State = 2;
        ox::event::SEvent event;
        event.EventType = ox::event::EET_USER_EVENT;
        event.UserEvent.UserData1 = 35;
        event.UserEvent.UserData2 = 0;
        event.UserEvent.UserData3 = 0;
        event.UserEvent.UserPointer = 0;
        ox::event::gp_subscriberList->OnEvent(event);
    }
}

void CDropshipEntity::locateNewAlienBulletTarget()
{
    const std::list<ox::entity::COxEntity*>& aliens = gp_entityManager->getEntityList(1);
    std::vector<ox::entity::COxEntity*> targets;
    for (std::list<ox::entity::COxEntity*>::const_iterator it = aliens.begin(); it != aliens.end(); ++it)
    {
        if ((*it)->getEntityType() == 6)
        {
            float x = Position.X - (*it)->getPosition().X;
            float y = Position.Y - (*it)->getPosition().Y;
            if (x * x + y * y <= 62500.0f)
                targets.push_back(*it);
        }
    }
    if (!targets.empty())
    {
        BulletTarget.Entity = targets[ox::algo::CRand::rand() % targets.size()];
        BulletTarget.Id = BulletTarget.Entity->getId();
        BulletTarget.UpdateCounter = gp_entityManager->getUpdateCounter();
    }
}

void CDropshipEntity::updateSprite(float frameDelta)
{
    if (Sprites[SpriteIndex])
        Sprites[SpriteIndex]->update(frameDelta);
}
int CDropshipEntity::onSpark(CSparkEntity* spark) { return -1; }
bool CDropshipEntity::wantsSpark() { return false; }
void CDropshipEntity::setDropshipState(int state) { State = state; }
int CDropshipEntity::getDropshipState() { return State; }

void CDropshipEntity::render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort)
{
    Color = ox::video::SColor(0xffffffff);
    if (Sprites[SpriteIndex])
    {
        ox::core::CPosition2d<float> position;
        position.X = Position.X - camera.X + viewPort.UpperLeftCorner.X;
        position.Y = Position.Y - camera.Y + viewPort.UpperLeftCorner.Y - Position.Z;
        Sprites[SpriteIndex]->drawRotated(position, SpriteAngle, 1.0f, Color);
    }
}
void CDropshipEntity::renderGroundLayer(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort)
{
    Beam.Start = ox::core::CPosition2d<float>(Position.X, Position.Y);
    Beam.End = ox::core::CPosition2d<float>(TargetPosition.X, TargetPosition.Y);
    Color = ox::video::SColor(0x80000000);
    if (Sprites[SpriteIndex])
    {
        ox::core::CPosition2d<float> position;
        position.X = Position.X - camera.X + viewPort.UpperLeftCorner.X;
        position.Y = Position.Y - camera.Y + viewPort.UpperLeftCorner.Y;
        Sprites[SpriteIndex]->drawRotated(position, SpriteAngle, 1.0f, Color);
    }
}
ox::video::ISpriteAnimationState* CDropshipEntity::getCurrentDisplaySprite() { return Sprites[SpriteIndex]; }
ox::core::CString<wchar_t> CDropshipEntity::getInfoString() { return ox::core::CString<wchar_t>(L""); }
ox::core::CString<wchar_t> CDropshipEntity::getMiniStatString() { return ox::core::CString<wchar_t>(L""); }
void CDropshipEntity::writeEntityData(ox::io::IWriteFile* file) {}
void CDropshipEntity::readEntityData(ox::io::IReadFile* file, int version) {}

CDropshipBulletEntity::CDropshipBulletEntity(float damage, const ox::core::CVector3d<float>& start,
    const ox::core::CVector3d<float>& target)
    : CEntity(0, 18, start.X, start.Y), TargetPosition(target), Hit(false), Beam(1.0f), LifeTime(0), Damage(damage)
{
    Position = start;
    Beam.Beam = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DropShipBullet"));
    Beam.EndSprite = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("DropShipBulletGlow"));
}
CDropshipBulletEntity::~CDropshipBulletEntity() {}

int CDropshipBulletEntity::updateLogic(float frameDelta)
{
    if (Hit)
        return 1;
    LifeTime += frameDelta;
    ox::core::CVector3d<float> direction = TargetPosition - Position;
    direction.normalize();
    ox::core::CVector3d<float> movement = direction * (frameDelta * 1800.0f);
    if ((movement.X < 0 && Position.X + movement.X < TargetPosition.X) ||
        (movement.X > 0 && Position.X + movement.X > TargetPosition.X) ||
        (movement.Y < 0 && Position.Y + movement.Y < TargetPosition.Y) ||
        (movement.Y > 0 && Position.Y + movement.Y > TargetPosition.Y))
    {
        Beam.End = ox::core::CPosition2d<float>(TargetPosition.X, TargetPosition.Y);
        Hit = true;
        gp_entityManager->appendEntity(new CParticleEntity(TargetPosition.X, TargetPosition.Y + 5.0f,
            TargetPosition.Z + 5.0f, 0, "GunHit"), 4);
        const std::list<ox::entity::COxEntity*>& aliens = gp_entityManager->getEntityList(1);
        for (std::list<ox::entity::COxEntity*>::const_iterator it = aliens.begin(); it != aliens.end(); ++it)
        {
            if ((*it)->getEntityType() == 6)
            {
                CAlienEntity* alien = (CAlienEntity*)*it;
                const ox::core::CVector3d<float>& position = alien->getPosition();
                float distance = ox::core::CMath::getSquaredDistance(ox::core::CPosition2d<float>(Position.X, Position.Y),
                    ox::core::CPosition2d<float>(position.X, position.Y));
                if (distance < 400.0f)
                {
                    float falloff = 20.0f - sqrtf(distance);
                    float damage = falloff * falloff * falloff / 8000.0f * Damage;
                    alien->dealDamage(damage, ox::core::CPosition2d<float>(Position.X, Position.Y), 3.0f, 6);
                }
            }
        }
    }
    else
    {
        Position += movement;
        Beam.End = ox::core::CPosition2d<float>(Position.X, Position.Y - Position.Z);
    }
    float length = ox::core::min_(LifeTime * 1800.0f, 120.0f);
    Beam.Start = Beam.End - ox::core::CPosition2d<float>(direction.X * length, (direction.Y - direction.Z) * length);
    return 0;
}
void CDropshipBulletEntity::updateSprite(float frameDelta)
{
    if (Beam.Beam)
        Beam.Beam->update(frameDelta);
}
bool CDropshipBulletEntity::addToRenderList(const ox::core::CRect<float>& visibleArea)
{
    return visibleArea.isPointInside(ox::core::CPosition2d<float>(Position.X, Position.Y - Position.Z)) ||
        visibleArea.isPointInside(ox::core::CPosition2d<float>(TargetPosition.X, TargetPosition.Y - TargetPosition.Z));
}
void CDropshipBulletEntity::render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort)
{
    gp_entityManager->renderEnergyBeam(&Beam, camera, viewPort);
}
void CDropshipBulletEntity::writeEntityData(ox::io::IWriteFile* file) {}
void CDropshipBulletEntity::readEntityData(ox::io::IReadFile* file, int version) {}

} // end namespace entity
} // end namespace harvest
