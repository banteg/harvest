// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include <algorithm>
#include "harvest/game/CWorld.h"
#include "harvest/game/CLuaManager.h"
#include "ox/core/CMath.h"
#include "harvest/entity/CMissileTurretEntity.h"
#include "harvest/entity/CAlienEntity.h"
#include "harvest/settings/CSystemConfig.h"
#include "harvest/settings/CAlienPriorities.h"
#include "ox/algo/CRand.h"
#include "ox/core/CBasic.h"
#include "ox/io/CHelpIO.h"
#include "ox/video/IParticlePackage.h"
#include "ox/video/IParticleState.h"
#include "ox/video/ISpritePackage.h"

namespace harvest {
namespace entity {

CMissileTurretEntity::CMissileTurretEntity(int type, float x, float y)
    : CBuildingEntity(g_nextEntityId++, type, x, y), Sparks(0), ReloadTime(12.0f), SpriteIndex(0),
      Kills(0), BurstCount(0), BurstTime(0), IdleTime(12.0f), Launching(false), TargetPosition(0, 0),
      TargetId(-1)
{
    switch (Type)
    {
    default:
        Sprites[0] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("MissileTower"));
        Sprites[1] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("MissileTowerOpening"));
        Sprites[2] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("MissileTowerClosing"));
        break;
    case 13:
        Sprites[0] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("Eagle"));
        Sprites[1] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("EagleOpening"));
        Sprites[2] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("EagleClosing"));
        break;
    case 14:
        Sprites[0] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("Tempest"));
        Sprites[1] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("TempestOpening"));
        Sprites[2] = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("TempestClosing"));
        break;
    }
}

CMissileTurretEntity::~CMissileTurretEntity()
{
    for (int i = 0; i < 3; ++i)
        if (Sprites[i])
            Sprites[i]->remove();
}

void CMissileTurretEntity::updateSprite(float frameDelta)
{
    if (Sprites[SpriteIndex] && Sprites[SpriteIndex]->update(frameDelta))
    {
        if (SpriteIndex == 1)
        {
            Launching = true;
            SpriteIndex = 2;
            Sprites[2]->reset();
        }
        else if (SpriteIndex == 2)
        {
            SpriteIndex = 0;
            Sprites[0]->reset();
        }
    }
}

int CMissileTurretEntity::getRequiredSparks()
{
    if (Type == 8)
        return 5;
    if (Type == 14)
        return 10;
    return 15;
}

CAlienEntity* CMissileTurretEntity::findPriorityAlien(const ox::core::CVector3d<float>& position,
    int prioritySet, float squaredRange, float minimumSquaredRange, ox::core::CVector3d<float>* nearest)
{
    float range = sqrtf(squaredRange);
    int xMin = gp_entityManager->calculateGridCoordinateClamp(position.X - range);
    int xMax = gp_entityManager->calculateGridCoordinateClamp(position.X + range);
    int yMin = gp_entityManager->calculateGridCoordinateClamp(position.Y - range);
    int yMax = gp_entityManager->calculateGridCoordinateClamp(position.Y + range);
    if (settings::g_attackPriorities[prioritySet].HoldFire)
        return 0;
    float nearestDistance = squaredRange * 16.0f;
    CAlienEntity* best;
    if (settings::g_attackPriorities[prioritySet].RangeIsImportant)
    {
        best = 0;
        int bestPriority = 1;
        float bestDistance = squaredRange;
        for (int x = xMin; x <= xMax; ++x)
            for (int y = yMin; y <= yMax; ++y)
            {
                std::list<CEntity*>& cell = gp_entityManager->Grid[1][y * 18 + x];
                for (std::list<CEntity*>::iterator it = cell.begin(); it != cell.end(); ++it)
                {
                    if ((*it)->getEntityType() != 6)
                        continue;
                    CAlienEntity* alien = static_cast<CAlienEntity*>(*it);
                    int type = alien->getAlienType();
                    if (settings::g_attackPriorities[prioritySet].Priorities[type] < bestPriority)
                        continue;
                    float dx = position.X - (*it)->getPosition().X;
                    float dy = position.Y - (*it)->getPosition().Y;
                    float distance = dx * dx + dy * dy;
                    if (distance <= squaredRange && distance >= minimumSquaredRange)
                    {
                        if (settings::g_attackPriorities[prioritySet].Priorities[type] > bestPriority)
                        {
                            bestPriority = settings::g_attackPriorities[prioritySet].Priorities[type];
                            best = static_cast<CAlienEntity*>(*it);
                            bestDistance = distance;
                        }
                        else if (distance < bestDistance)
                        {
                            best = static_cast<CAlienEntity*>(*it);
                            bestDistance = distance;
                        }
                    }
                    else if (distance < nearestDistance && nearest)
                    {
                        *nearest = (*it)->getPosition();
                        nearestDistance = distance;
                    }
                }
            }
    }
    else
    {
        int bestPriority = 1;
        std::vector<ox::entity::COxEntity*> candidates;
        for (int x = xMin; x <= xMax; ++x)
            for (int y = yMin; y <= yMax; ++y)
            {
                std::list<CEntity*>& cell = gp_entityManager->Grid[1][y * 18 + x];
                for (std::list<CEntity*>::iterator it = cell.begin(); it != cell.end(); ++it)
                {
                    if ((*it)->getEntityType() != 6)
                        continue;
                    CAlienEntity* alien = static_cast<CAlienEntity*>(*it);
                    int type = alien->getAlienType();
                    if (settings::g_attackPriorities[prioritySet].Priorities[type] < bestPriority)
                        continue;
                    float dx = position.X - (*it)->getPosition().X;
                    float dy = position.Y - (*it)->getPosition().Y;
                    float distance = dx * dx + dy * dy;
                    if (distance <= squaredRange && distance >= minimumSquaredRange)
                    {
                        if (settings::g_attackPriorities[prioritySet].Priorities[type] > bestPriority)
                        {
                            candidates.clear();
                            bestPriority = settings::g_attackPriorities[prioritySet].Priorities[type];
                        }
                        candidates.push_back(*it);
                    }
                    else if (distance < nearestDistance && nearest)
                    {
                        *nearest = (*it)->getPosition();
                        nearestDistance = distance;
                    }
                }
            }
        if (!candidates.empty())
            best = static_cast<CAlienEntity*>(candidates[ox::algo::CRand::rand() % candidates.size()]);
        else
            best = 0;
    }
    return best;
}

int CMissileTurretEntity::updateLogic(float frameDelta)
{
    if (Sparks >= getRequiredSparks())
    {
        if (!Launching && SpriteIndex != 1)
        {
            ReloadTime -= frameDelta;
            if (ReloadTime <= 0)
            {
                CAlienEntity* alien = findPriorityAlien();
                if (!alien)
                    ReloadTime += 2.0f;
                else
                {
                    if (Type == 14)
                    {
                        const ox::core::CVector3d<float>& target = alien->getPosition();
                        TargetPosition.X = target.X;
                        TargetPosition.Y = target.Y;
                        BurstTime = 0;
                        BurstCount = 4;
                        SpriteIndex = 1;
                        Sprites[1]->reset();
                    }
                    else if (Type == 13)
                    {
                        const ox::core::CVector3d<float>& target = alien->getPosition();
                        TargetPosition.X = target.X;
                        TargetPosition.Y = target.Y;
                        TargetId = alien->getId();
                        SpriteIndex = 1;
                        Sprites[1]->reset();
                        BurstTime = 0;
                        BurstCount = 3;
                    }
                    else
                    {
                        // These draws are present in both originals even though their results are unused.
                        ox::algo::CRand::rand();
                        ox::algo::CRand::rand();
                        ox::algo::CRand::rand();
                        ox::algo::CRand::rand();
                        const ox::core::CVector3d<float>& target = alien->getPosition();
                        TargetPosition.X = target.X;
                        TargetPosition.Y = target.Y;
                        ox::core::CVector2d<float> direction(alien->getMovementTarget().X - TargetPosition.X,
                            alien->getMovementTarget().Y - TargetPosition.Y);
                        float dx = direction.X;
                        float dy = direction.Y;
                        float ax = ox::core::abs_(dx);
                        float ay = ox::core::abs_(dy);
                        if (ax != 0 || ay != 0)
                        {
                            float length = ax > ay ? ax - ay + ay * 1.5f : ay - ax + ax * 1.5f;
                            float lead = ox::core::max_(1.2f, length / 500.0f * 3.5f) * 10.0f;
                            float magnitude = ox::core::CVector2d<float>(dx, dy).getLength();
                            TargetPosition.X += dx / magnitude * lead;
                            TargetPosition.Y += dy / magnitude * lead;
                        }
                        SpriteIndex = 1;
                        Sprites[1]->reset();
                    }
                    float reload;
                    if (Type == 14)
                        reload = 20.0f;
                    else if (Type == 13)
                        reload = 30.0f;
                    else
                        reload = 12.0f;
                    ReloadTime += reload;
                }
            }
        }
    }
    if (Launching)
    {
        switch (Type)
        {
        case 8:
            gp_entityManager->appendEntity(new CMissileEntity(Position.X, Position.Y + 1.0f,
                TargetPosition, Id, 0, -1), 3);
            if (game::gp_luaManager)
                game::gp_luaManager->hookMissileLaunched(0, this, -1, TargetPosition.X, TargetPosition.Y);
            Sparks = 0;
            Launching = false;
            SpriteIndex = 2;
            Sprites[2]->reset();
            break;
        case 13:
            if (BurstTime > 0)
                BurstTime -= frameDelta;
            else if (BurstCount == 0)
            {
                Launching = false;
                Sparks = 0;
                SpriteIndex = 2;
                Sprites[2]->reset();
            }
            else
            {
                --BurstCount;
                BurstTime += 0.3f;
                gp_entityManager->appendEntity(new CMissileEntity(Position.X, Position.Y,
                    TargetPosition, Id, 2, TargetId), 3);
                gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y, 5.0f,
                    0, "EagleLaunch"), 4);
                if (game::gp_luaManager)
                    game::gp_luaManager->hookMissileLaunched(2, this, TargetId,
                        TargetPosition.X, TargetPosition.Y);
            }
            break;
        case 14:
            if (BurstTime > 0)
                BurstTime -= frameDelta;
            else if (BurstCount == 0)
            {
                Launching = false;
                Sparks = 0;
                SpriteIndex = 2;
                Sprites[2]->reset();
            }
            else
            {
                --BurstCount;
                BurstTime += 0.2f;
                int x = ox::algo::CRand::rand();
                int y = ox::algo::CRand::rand();
                ox::core::CPosition2d<float> target(TargetPosition.X + float(x % 200 - 100),
                    TargetPosition.Y + float(y % 200 - 100));
                gp_entityManager->appendEntity(new CMissileEntity(Position.X, Position.Y,
                    target, Id, 1, -1), 3);
                gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y + 1.0f,
                    5.0f, 0, "MirvLaunch"), 4);
                if (game::gp_luaManager)
                    game::gp_luaManager->hookMissileLaunched(1, this, -1,
                        TargetPosition.X, TargetPosition.Y);
            }
            break;
        }
    }
    if (Sparks >= getRequiredSparks())
    {
        Color = ox::video::SColor(0xffffffff);
        IdleTime = 0;
    }
    else
    {
        IdleTime += frameDelta;
        int brightness = int(255.0f - IdleTime * IdleTime);
        brightness = ox::core::clamp(brightness, 128, 255);
        Color = ox::video::SColor(255, brightness, brightness, brightness);
    }
    return 0;
}


CAlienEntity* CMissileTurretEntity::findPriorityAlien()
{
    if (Type == 14)
        return findPriorityAlien(Position, 4, 250000.0f, 900.0f, 0);
    if (Type == 13)
        return findPriorityAlien(Position, 3, 2250000.0f, 900.0f, 0);
    return findPriorityAlien(Position, 2, 250000.0f, 900.0f, 0);
}

int CMissileTurretEntity::onSpark(CSparkEntity* spark)
{
    if (Sparks >= getRequiredSparks())
        return -1;
    ++Sparks;
    return 0;
}

ox::core::CString<wchar_t> CMissileTurretEntity::getInfoString()
{
    ox::core::CString<wchar_t> result;
    if (Sparks < getRequiredSparks())
        result = settings::gp_systemConfig->getLocalizedText(L"entity:turretEnergy",
            int(float(Sparks) / float(getRequiredSparks()) * 100.0f));
    else if (Launching || SpriteIndex == 1)
        result = settings::gp_systemConfig->getLocalizedText(L"entity:turretLaunching");
    else if (ReloadTime > 2.0f)
    {
        float reload = Type == 14 ? 18.0f : Type == 13 ? 28.0f : 10.0f;
        result = settings::gp_systemConfig->getLocalizedText(L"entity:turretReloading",
            int((1.0f - (ReloadTime - 2.0f) / reload) * 100.0f));
    }
    else
        result = settings::gp_systemConfig->getLocalizedText(L"entity:turretScanning");
    return ox::core::CString<wchar_t>(result);
}

ox::core::CString<wchar_t> CMissileTurretEntity::getMiniStatString()
{
    if (Kills == 1)
        return settings::gp_systemConfig->getLocalizedText(L"entity:alienKill", Kills);
    return settings::gp_systemConfig->getLocalizedText(L"entity:alienKills", Kills);
}

ox::core::CString<wchar_t> CMissileTurretEntity::getOperatorString()
{
    return ox::core::CString<wchar_t>(L"");
}

bool CMissileTurretEntity::wantsSpark()
{
    return Sparks < getRequiredSparks();
}

void CMissileTurretEntity::render(const ox::core::CPosition2d<float>& camera,
    const ox::core::CRect<int>& viewPort)
{
    CEntity::renderSpriteFixed(camera, viewPort, Sprites[SpriteIndex]);
    int required = getRequiredSparks();
    if (Sparks < required)
    {
        int sparks = ox::core::max_(0, Sparks);
        float progress = float(sparks) / float(required);
        renderSelfProgress(camera, viewPort, progress, ENERGY_PROGRESS_COLOR);
    }
}

ox::video::ISpriteAnimationState* CMissileTurretEntity::getCurrentDisplaySprite()
{
    return Sprites[SpriteIndex];
}

void CMissileTurretEntity::writeEntityData(ox::io::IWriteFile* file)
{
    ox::io::CHelpIO::writeInt(file, Sparks);
    ox::io::CHelpIO::writeFloat(file, ReloadTime);
    ox::io::CHelpIO::writeInt(file, Kills);
    ox::io::CHelpIO::writeInt(file, TargetId);
    ox::io::CHelpIO::writeInt(file, SpriteIndex);
    Sprites[SpriteIndex]->write(file);
    ox::io::CHelpIO::writeFloat(file, IdleTime);
    ox::io::CHelpIO::writeInt(file, BurstCount);
    ox::io::CHelpIO::writeFloat(file, BurstTime);
}

void CMissileTurretEntity::readEntityData(ox::io::IReadFile* file, int version)
{
    if (version <= 8)
        ox::io::CHelpIO::readInt(file);
    Sparks = ox::io::CHelpIO::readInt(file);
    ReloadTime = ox::io::CHelpIO::readFloat(file);
    if (version >= 2)
        Kills = ox::io::CHelpIO::readInt(file);
    if (version >= 9)
    {
        TargetId = ox::io::CHelpIO::readInt(file);
        SpriteIndex = ox::io::CHelpIO::readInt(file);
        Sprites[SpriteIndex]->read(file);
        IdleTime = ox::io::CHelpIO::readFloat(file);
    }
    if (version >= 12)
    {
        BurstCount = ox::io::CHelpIO::readInt(file);
        BurstTime = ox::io::CHelpIO::readFloat(file);
    }
}

CMissileEntity::CMissileEntity(float x, float y, const ox::core::CPosition2d<float>& target,
    int ownerId, int missileType, int targetId)
    : CEntity(0, 9, x, y), TargetPosition(target.X, target.Y, 5.0f), Speed(0, 0, 100.0f),
      OwnerId(ownerId), MissileType(missileType), LifeTime(30.0f), Retargeting(true),
      Particle(0)
{
    Position.Z = 10.0f;
    Target.Id = targetId;
    initializeMissileType();
}

void CMissileEntity::initializeMissileType()
{
    if (Particle)
        Particle->remove();
    switch (MissileType)
    {
    default:
        if (gp_particlePackage)
            Particle = gp_particlePackage->addNewParticleState(ox::core::CString<char>("Missile"));
        break;
    case 2:
        if (gp_particlePackage)
        {
            Particle = gp_particlePackage->addNewParticleState(ox::core::CString<char>("Eagle"));
            Speed.Z = 300.0f;
            Position.Z = 15.0f;
        }
        break;
    case 1:
        if (gp_particlePackage)
        {
            Particle = gp_particlePackage->addNewParticleState(ox::core::CString<char>("Mirv"));
            Speed.Z = 200.0f;
        }
        break;
    }
}

void CMissileEntity::setSpeed(float x, float y, float z)
{
    Speed.X = x;
    Speed.Y = y;
    Speed.Z = z;
}

void CMissileEntity::disableRetargeting()
{
    Retargeting = false;
}

CMissileEntity::~CMissileEntity()
{
    if (Particle)
        Particle->remove();
}

int CMissileEntity::updateLogic(float frameDelta)
{
    if (MissileType == 2)
    {
        LifeTime -= frameDelta;
        if (LifeTime <= 0)
        {
            gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y, Position.Z,
                &Speed, "EagleDead"), 4);
            return 1;
        }
        gp_entityManager->updateReference(Target, 1, true);
        if (!Target.Entity)
        {
            if (!Retargeting)
            {
                gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y, Position.Z,
                    &Speed, "EagleDead"), 4);
                return 1;
            }
            Target.Entity = CMissileTurretEntity::findPriorityAlien(Position, 3, 2250000.0f, 0.0f, 0);
            if (!Target.Entity)
            {
                gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y, Position.Z,
                    &Speed, "EagleDead"), 4);
                return 1;
            }
            Target.Id = Target.Entity->getId();
            Target.UpdateCounter = gp_entityManager->getUpdateCounter();
        }
        if (Target.Entity)
        {
            const ox::core::CVector3d<float>& target = Target.Entity->getPosition();
            TargetPosition.X = target.X;
            TargetPosition.Y = target.Y;
            float dx = TargetPosition.X - Position.X;
            float dy = TargetPosition.Y - Position.Y;
            TargetPosition.Z = ox::core::min_(ox::core::max_(target.Z,
                sqrtf(dx * dx + dy * dy) * 0.5f), 300.0f);
            ox::core::CVector3d<float> direction(dx, dy, TargetPosition.Z - Position.Z);
            if (ox::core::abs_(direction.X) < 15.0f && ox::core::abs_(direction.Y) < 15.0f &&
                ox::core::abs_(direction.Z) < 15.0f)
            {
                float damage = 50.0f;
                CAlienEntity* alien = static_cast<CAlienEntity*>(Target.Entity);
                if (alien->dealDamage(damage, ox::core::CPosition2d<float>(Position.X, Position.Y), 5.0f, 2) && OwnerId > 0)
                {
                    ox::entity::COxEntity* owner = gp_entityManager->locateEntity(OwnerId, 0);
                    if (owner && owner->getEntityType() == 13)
                        static_cast<CMissileTurretEntity*>(owner)->addKillCount(1);
                }
                gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y, Position.Z,
                    0, "EagleExplosion"), 4);
                return 1;
            }
            if (Position.Z < 1.0f)
                Position.Z = 1.0f;
            updateSpeed(direction, frameDelta, 500.0f, 300.0f, 2.5f, 2.5f);
        }
    }
    else
    {
        float dx = TargetPosition.X - Position.X;
        float dy = TargetPosition.Y - Position.Y;
        if (MissileType == 0 && Position.Z < 5.0f)
        {
            const std::list<ox::entity::COxEntity*>& entities = gp_entityManager->getEntityList(1);
            std::vector<SAlienDistancePair> aliens;
            for (std::list<ox::entity::COxEntity*>::const_iterator it = entities.begin();
                it != entities.end(); ++it)
            {
                if ((*it)->getEntityType() != 6)
                    continue;
                CAlienEntity* alien = static_cast<CAlienEntity*>(*it);
                const ox::core::CVector3d<float>& alienPosition = alien->getPosition();
                float distance = ox::core::CMath::getSquaredDistance(ox::core::CPosition2d<float>(Position.X, Position.Y), ox::core::CPosition2d<float>(alienPosition.X, alienPosition.Y));
                if (distance < 10000.0f)
                {
                    SAlienDistancePair pair;
                    pair.Alien = alien;
                    pair.SquaredDistance = distance;
                    aliens.push_back(pair);
                }
            }
            if (!aliens.empty())
            {
                std::sort(aliens.begin(), aliens.end(), SAlienDistanceSorter());
                int kills = 0;
                for (unsigned int i = 0; i < 7; ++i)
                {
                    if (i >= aliens.size())
                        break;
                    float falloff = float(100.0 - sqrt(double(aliens[i].SquaredDistance)));
                    float damage = falloff * falloff * falloff / 1000000.0f * 120.0f;
                    if (aliens[i].Alien->dealDamage(damage,
                            ox::core::CPosition2d<float>(Position.X, Position.Y), 3.0f, 1))
                        ++kills;
                }
                if (kills > 0 && OwnerId >= 0)
                {
                    ox::entity::COxEntity* owner = gp_entityManager->locateEntity(OwnerId, 0);
                    if (owner && owner->getEntityType() == 8)
                        static_cast<CMissileTurretEntity*>(owner)->addKillCount(kills);
                }
            }
            gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y, 1.0f,
                0, "MissleExplosion"), 4);
            return 1;
        }
        if (MissileType == 1 && Position.Z < 50.0f && Speed.Z < 0)
        {
            const std::list<ox::entity::COxEntity*>& entities = gp_entityManager->getEntityList(1);
            std::vector<CAlienEntity*> aliens;
            for (std::list<ox::entity::COxEntity*>::const_iterator it = entities.begin();
                it != entities.end(); ++it)
            {
                if ((*it)->getEntityType() != 6)
                    continue;
                CAlienEntity* alien = static_cast<CAlienEntity*>(*it);
                const ox::core::CVector3d<float>& alienPosition = alien->getPosition();
                float distance = ox::core::CMath::getSquaredDistance(ox::core::CPosition2d<float>(Position.X, Position.Y), ox::core::CPosition2d<float>(alienPosition.X, alienPosition.Y));
                if (distance < 10000.0f)
                    aliens.push_back(alien);
            }
            if (!aliens.empty())
                for (int i = 0; i < 3; ++i)
                {
                    CAlienEntity* alien = aliens[ox::algo::CRand::rand() % aliens.size()];
                    gp_entityManager->appendEntity(new CTempestBlastEntity(Position,
                        alien->getPosition(), alien->getId(), OwnerId), 3);
                }
            gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y, Position.Z,
                0, "MirvExplosion"), 4);
            return 1;
        }
        TargetPosition.Z = sqrtf(dx * dx + dy * dy) * 0.5f;
        if (MissileType == 1)
            updateSpeed(ox::core::CVector3d<float>(dx, dy, TargetPosition.Z - Position.Z),
                frameDelta, 250.0f, 200.0f, 0.1f, 0.01f);
        else
            updateSpeed(ox::core::CVector3d<float>(dx, dy, TargetPosition.Z - Position.Z),
                frameDelta, 150.0f, 100.0f, 0.004f, 0.001f);
    }
    Position.X += Speed.X * frameDelta;
    Position.Y += Speed.Y * frameDelta;
    Position.Z += Speed.Z * frameDelta;
    ox::core::CVector3d<float> position(Position);
    if (Particle && !Particle->update(frameDelta, position))
    {
        Particle->remove();
        Particle = 0;
    }
    return 0;
}


void CMissileEntity::updateSpeed(const ox::core::CVector3d<float>& direction, float frameDelta,
    float acceleration, float speedLimit, float drag, float verticalDrag)
{
    if (direction.X < 0)
        Speed.X -= acceleration * frameDelta;
    else
        Speed.X += acceleration * frameDelta;
    if (direction.Y < 0)
        Speed.Y -= acceleration * frameDelta;
    else
        Speed.Y += acceleration * frameDelta;
    if (direction.Z < 0)
        Speed.Z -= acceleration * frameDelta;
    else
        Speed.Z += acceleration * frameDelta;
    Speed.X -= Speed.X * drag * frameDelta;
    Speed.Y -= drag * Speed.Y * frameDelta;
    Speed.Z -= verticalDrag * Speed.Z * frameDelta;
    float minimum = -speedLimit;
    Speed.X = !(Speed.X > speedLimit)
        ? ox::core::max_(minimum, Speed.X) : speedLimit;
    Speed.Y = !(Speed.Y > speedLimit)
        ? ox::core::max_(minimum, Speed.Y) : speedLimit;
    Speed.Z = !(Speed.Z > speedLimit)
        ? ox::core::max_(minimum, Speed.Z) : speedLimit;
}

bool CMissileEntity::addToRenderList(const ox::core::CRect<float>& visibleArea)
{
    if (visibleArea.isPointInside(ox::core::CPosition2d<float>(Position.X, Position.Y)))
        return true;
    return visibleArea.isPointInside(ox::core::CPosition2d<float>(Position.X, Position.Y - Position.Z));
}

void CMissileEntity::render(const ox::core::CPosition2d<float>& camera,
    const ox::core::CRect<int>& viewPort)
{
    if (!Particle)
        return;
    ox::core::CPosition2d<float> position;
    position.X = Position.X - camera.X + float(viewPort.UpperLeftCorner.X);
    position.Y = Position.Y - camera.Y + float(viewPort.UpperLeftCorner.Y);
    Particle->render2DShadow(position, 1.0f, 0.5f);
    position.Y -= Position.Z;
    Particle->render2D(position, 1.0f);
}

void CMissileEntity::writeEntityData(ox::io::IWriteFile* file)
{
    ox::io::CHelpIO::writeFloat(file, TargetPosition.X);
    ox::io::CHelpIO::writeFloat(file, TargetPosition.Y);
    ox::io::CHelpIO::writeFloat(file, TargetPosition.Z);
    ox::io::CHelpIO::writeFloat(file, Speed.X);
    ox::io::CHelpIO::writeFloat(file, Speed.Y);
    ox::io::CHelpIO::writeFloat(file, Speed.Z);
    ox::io::CHelpIO::writeFloat(file, Position.Z);
    ox::io::CHelpIO::writeInt(file, OwnerId);
    ox::io::CHelpIO::writeInt(file, MissileType);
    ox::io::CHelpIO::writeFloat(file, LifeTime);
}

void CMissileEntity::readEntityData(ox::io::IReadFile* file, int version)
{
    TargetPosition.X = ox::io::CHelpIO::readFloat(file);
    TargetPosition.Y = ox::io::CHelpIO::readFloat(file);
    TargetPosition.Z = ox::io::CHelpIO::readFloat(file);
    Speed.X = ox::io::CHelpIO::readFloat(file);
    Speed.Y = ox::io::CHelpIO::readFloat(file);
    Speed.Z = ox::io::CHelpIO::readFloat(file);
    Position.Z = ox::io::CHelpIO::readFloat(file);
    if (version >= 6)
        OwnerId = ox::io::CHelpIO::readInt(file);
    if (version >= 7)
    {
        MissileType = ox::io::CHelpIO::readInt(file);
        LifeTime = ox::io::CHelpIO::readFloat(file);
    }
    initializeMissileType();
}

CTempestBlastEntity::CTempestBlastEntity(const ox::core::CVector3d<float>& position,
    const ox::core::CVector3d<float>& target, int targetId, int ownerId)
    : CEntity(0, 15, position.X, position.Y), TargetPosition(target), TargetId(targetId),
      OwnerId(ownerId), BeamTime(0), LifeTime(0.5f), FirstUpdate(true)
{
    Sprite = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("MirvRay"));
    Position = position;
}

CTempestBlastEntity::~CTempestBlastEntity()
{
    if (Sprite)
        Sprite->remove();
    deleteBeams();
}

void CTempestBlastEntity::deleteBeams()
{
    for (unsigned int i = 0; i < Beams.size(); ++i)
    {
        Beams[i]->Beam = 0;
        delete Beams[i];
    }
    Beams.clear();
}

int CTempestBlastEntity::updateLogic(float frameDelta)
{
    LifeTime -= frameDelta;
    if (LifeTime < 0)
        return 1;
    if (FirstUpdate)
    {
        FirstUpdate = false;
        CAlienEntity* alien = (CAlienEntity*)gp_entityManager->locateEntity(TargetId, 1);
        if (alien)
        {
            float damage = 30.0f;
            if (alien->dealDamage(damage, ox::core::CPosition2d<float>(Position.X, Position.Y), 0.1f, 3))
            {
                CEntity* tower = (CEntity*)gp_entityManager->locateEntity(OwnerId, 0);
                if (tower && tower->getEntityType() == 14)
                    ((CMissileTurretEntity*)tower)->addKillCount(1);
            }
        }
        gp_entityManager->appendEntity(new CParticleEntity(TargetPosition.X,
            TargetPosition.Y + 1.0f, TargetPosition.Z + 1.0f, 0, "MirvHit"), 4);
    }
    BeamTime -= frameDelta;
    if (BeamTime <= 0)
    {
        BeamTime += 0.05f;
        int phase = int((LifeTime * -2.0f + 1.0f) * 5.0f);
        deleteBeams();
        createBeams(Position, TargetPosition, phase);
    }
    return 0;
}

void CTempestBlastEntity::createBeams(const ox::core::CVector3d<float>& start,
    const ox::core::CVector3d<float>& end, int phase)
{
    ox::core::CVector3d<float> direction = end - start;
    SEnergyBeam* beam = new SEnergyBeam(phase * -1.5f + 8.0f);
    beam->Start = ox::core::CPosition2d<float>(start.X, start.Y - start.Z);
    beam->Beam = Sprite;
    beam->Color = ox::video::SColor(255 - phase * 30, 255, 255, 255);
    float length = 26.0f - float(phase * 3);
    if (ox::core::abs_(direction.X) < length && ox::core::abs_(direction.Y) < length &&
        ox::core::abs_(direction.Z) < length)
        beam->End = ox::core::CPosition2d<float>(end.X, end.Y - end.Z);
    else
    {
        length += ox::algo::CRand::rand() % (20 - phase * 2);
        direction.normalize();
        direction *= length;
        if (length > 2.0f)
        {
            int randomRange = int(length + 0.5f);
            int halfRange = randomRange >> 1;
            direction.X += (ox::algo::CRand::rand() % randomRange) - halfRange;
            direction.Y += (ox::algo::CRand::rand() % randomRange) - halfRange;
            direction.Z += (ox::algo::CRand::rand() % randomRange) - halfRange;
        }
        direction += start;
        beam->End = ox::core::CPosition2d<float>(direction.X, direction.Y - direction.Z);
        createBeams(direction, end, phase);
    }
    Beams.push_back(beam);
}

void CTempestBlastEntity::updateSprite(float frameDelta)
{
    if (Sprite)
        Sprite->update(frameDelta);
}

bool CTempestBlastEntity::addToRenderList(const ox::core::CRect<float>& visibleArea)
{
    if (visibleArea.isPointInside(ox::core::CPosition2d<float>(Position.X, Position.Y)))
        return true;
    return visibleArea.isPointInside(ox::core::CPosition2d<float>(TargetPosition.X, TargetPosition.Y));
}

void CTempestBlastEntity::render(const ox::core::CPosition2d<float>& camera,
    const ox::core::CRect<int>& viewPort)
{
    for (unsigned int i = 0; i < Beams.size(); ++i)
        gp_entityManager->insertTopLevelEnergyBeam(Beams[i]);
}

void CTempestBlastEntity::writeEntityData(ox::io::IWriteFile* file)
{
    ox::io::CHelpIO::writeFloat(file, Position.Z);
    ox::io::CHelpIO::writeFloat(file, TargetPosition.X);
    ox::io::CHelpIO::writeFloat(file, TargetPosition.Y);
    ox::io::CHelpIO::writeFloat(file, TargetPosition.Z);
    ox::io::CHelpIO::writeFloat(file, BeamTime);
    ox::io::CHelpIO::writeFloat(file, LifeTime);
    ox::io::CHelpIO::writeInt(file, TargetId);
    ox::io::CHelpIO::writeInt(file, OwnerId);
    ox::io::CHelpIO::writeInt(file, FirstUpdate);
}

void CTempestBlastEntity::readEntityData(ox::io::IReadFile* file, int version)
{
    Position.Z = ox::io::CHelpIO::readFloat(file);
    TargetPosition.X = ox::io::CHelpIO::readFloat(file);
    TargetPosition.Y = ox::io::CHelpIO::readFloat(file);
    TargetPosition.Z = ox::io::CHelpIO::readFloat(file);
    BeamTime = ox::io::CHelpIO::readFloat(file);
    LifeTime = ox::io::CHelpIO::readFloat(file);
    TargetId = ox::io::CHelpIO::readInt(file);
    OwnerId = ox::io::CHelpIO::readInt(file);
    FirstUpdate = ox::io::CHelpIO::readInt(file);
}

} // end namespace entity
} // end namespace harvest
