// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

// The object has an iostream static initializer, set up before the game headers' statics; the
// header that pulled it in is not identified yet. The world grid offset is initialized before
// ENERGY_PROGRESS_COLOR, so CWorld.h comes first.
#include <iostream>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CMinerEntity.h"
#include "harvest/ECustomEvents.h"
#include "harvest/entity/CMineralsEntity.h"
#include "harvest/game/CLuaManager.h"
#include "harvest/game/CStatistics.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/core/CBasic.h"
#include "ox/event/IEventReceiver.h"
#include "ox/io/CHelpIO.h"
#include "ox/video/IParticlePackage.h"
#include "ox/video/IParticleState.h"
#include "ox/video/ISpritePackage.h"

namespace harvest {
namespace entity {

CMineralGatherEntity::CMineralGatherEntity(float x, float y)
    : CBuildingEntity(g_nextEntityId++, 4, x, y), Active(false), MiningTime(0), Cooldown(0), IdleTimer(0),
      OutOfMinerals(false), MineralsMined(0), MiningParticle(0), Beam(5.0f)
{
    Sprite = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("MineralGatherer"));
    Beam.Beam = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("MiningLaser"));
    Beam.StartSprite = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("MiningGlow"));
    Beam.EndSprite = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("MiningSprite"));
    Beam.Start = ox::core::CPosition2d<float>(Position.X, Position.Y - 24.0f);
}

CMineralGatherEntity::~CMineralGatherEntity()
{
    if (MiningParticle)
        MiningParticle->remove();
    if (Sprite)
        Sprite->remove();
}

int CMineralGatherEntity::updateLogic(float frameDelta)
{
    if (Cooldown > 0)
        Cooldown -= frameDelta;
    if (IdleTimer > 0)
        IdleTimer -= frameDelta;

    if (Active && Cooldown <= 0 && !OutOfMinerals)
    {
        gp_entityManager->updateReference(Target, 0, true);
        if (!Target.Entity)
        {
            Target.Entity = locateFreeMinerals();
            if (Target.Entity)
            {
                MiningParticle = gp_particlePackage->addNewParticleState(ox::core::CString<char>("Mining"));
                Target.Id = Target.Entity->getId();
                Target.UpdateCounter = gp_entityManager->getUpdateCounter();
            }
            else if (MiningParticle)
            {
                MiningParticle->remove();
                MiningParticle = 0;
                Cooldown = 3.0f;
            }
        }

        if (Target.Entity)
        {
            Beam.End = ox::core::CPosition2d<float>(Target.Entity->getPosition().X, Target.Entity->getPosition().Y);

            // the beam fades in, and out over its last half second
            if (MiningTime < 4.2f - 0.5f)
                Beam.Color = ox::video::SColor(ox::core::clamp((int)(MiningTime * 255.0f), 32, 255),
                    255, 255, 255);
            else
                Beam.Color = ox::video::SColor(ox::core::clamp((int)((4.2f - MiningTime) * 511.0f), 32, 255),
                    255, 255, 255);
        }

        MiningTime += frameDelta;
        if (MiningTime >= 4.2f && Target.Entity)
        {
            int amount = ((CMineralsEntity*)Target.Entity)->withdrawAmount(1);
            game::gp_mineralAmount->modifyValue(amount);
            game::gp_negatedMineralAmount->modifyValue(-amount);
            MineralsMined += amount;
            if (game::gp_statistics)
                game::gp_statistics->modifyLevelStatValue(2, (float)amount);

            if (MineralsMined == 1 && game::gp_world->getGameMode() == 0)
            {
                ox::event::SEvent event;
                event.EventType = ox::event::EET_USER_EVENT;
                event.UserEvent.UserData1 = ECE_TUTORIAL_HINT;
                // the hint for the first mined mineral
                event.UserEvent.UserData2 = 25;
                event.UserEvent.UserData3 = 0;
                event.UserEvent.UserPointer = 0;
                ox::event::gp_subscriberList->OnEvent(event);
            }

            if (game::gp_luaManager)
                game::gp_luaManager->hookCreditsMined(this, Target.Entity->getId());

            Active = false;
            Target.Entity = 0;
            Target.Id = -1;
            if (MiningParticle)
            {
                MiningParticle->remove();
                MiningParticle = 0;
            }
            MiningTime = 0;
            IdleTimer = 12.0f;
        }
    }
    else if (OutOfMinerals)
    {
        if (IdleTimer <= 0)
            IdleTimer = 2.0f;
        // a slow pulse
        Beam.Color = ox::video::SColor((int)(sin(IdleTimer * 0.5f * 3.14159265f) * 200.0) + 55, 255, 255, 255);
    }

    if (Active)
        Color = ox::video::SColor(0xffffffff);
    else
    {
        // idle harvesters fade to grey
        int brightness = (int)(IdleTimer / 12.0f * 255.0f);
        if (brightness > 255)
            brightness = 255;
        else if (brightness < 128)
            brightness = 128;
        Color = ox::video::SColor(255, brightness, brightness, brightness);
    }

    if (MiningParticle)
    {
        if (Target.Entity)
        {
            Beam.End = ox::core::CPosition2d<float>(Target.Entity->getPosition().X, Target.Entity->getPosition().Y);
            Beam.End.X += (int)(cos(MiningTime) * 5.0);
            Beam.End.Y += (int)(sin(MiningTime) * 4.0);
        }

        ox::core::CVector3d<float> position(Beam.End.X, Beam.End.Y, 1.0f);
        if (!MiningParticle->update(frameDelta, position))
        {
            MiningParticle->remove();
            MiningParticle = 0;
        }
    }

    return 0;
}

CMineralsEntity* CMineralGatherEntity::locateFreeMinerals()
{
    CMineralsEntity* minerals = (CMineralsEntity*)gp_entityManager->findRandomEntityInRange(
        ox::core::CPosition2d<float>(Position.X, Position.Y), 10000.0f, 0, 5);

    if (minerals)
    {
        if (minerals->getHoggerId() >= 0)
            minerals = 0;
    }
    else
    {
        OutOfMinerals = true;
        IdleTimer = 0;
        if (game::gp_luaManager)
            game::gp_luaManager->hookMinerOutOfMinerals(this);
    }

    return minerals;
}

void CMineralGatherEntity::updateSprite(float frameDelta)
{
    if (Beam.Beam)
        Beam.Beam->update(frameDelta);
    if (OutOfMinerals && Beam.StartSprite)
        Beam.StartSprite->update(frameDelta);
    if (Sprite)
        Sprite->update(frameDelta);
}

int CMineralGatherEntity::onSpark(CSparkEntity* spark)
{
    if (Active)
        return -1;
    MiningTime = 0;
    Active = true;
    return 0;
}

bool CMineralGatherEntity::wantsSpark()
{
    return !Active;
}

bool CMineralGatherEntity::isAbleToHarvest()
{
    if (OutOfMinerals)
        return false;
    if (IdleTimer > 0)
        return true;
    return Active;
}

bool CMineralGatherEntity::hasMoreMinerals()
{
    return !OutOfMinerals;
}

void CMineralGatherEntity::notifyMineralsAppeared()
{
    if (!OutOfMinerals || !locateFreeMinerals())
        return;

    OutOfMinerals = false;
    if (Beam.StartSprite)
        Beam.StartSprite->reset();
}

void CMineralGatherEntity::render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort)
{
    if (Active && Target.Entity && Beam.Beam)
        gp_entityManager->insertTopLevelEnergyBeam(&Beam);

    CEntity::renderSpriteFixed(camera, viewPort, Sprite);

    if (OutOfMinerals && Beam.StartSprite)
    {
        ox::core::CPosition2d<float> pos;
        pos.X = Position.X - camera.X + viewPort.UpperLeftCorner.X;
        pos.Y = Position.Y - camera.Y + viewPort.UpperLeftCorner.Y - 24.0f;
        Beam.StartSprite->drawScaled(pos, 1.0f, Beam.Color);
    }
}

ox::video::ISpriteAnimationState* CMineralGatherEntity::getCurrentDisplaySprite()
{
    return Sprite;
}

ox::core::CString<wchar_t> CMineralGatherEntity::getInfoString()
{
    if (OutOfMinerals)
        return settings::gp_systemConfig->getLocalizedText(L"entity:harvesterAreaCleared");
    if (Active && Target.Entity)
        return settings::gp_systemConfig->getLocalizedText(L"entity:harvesterHarvesting");
    if (!Active)
        return settings::gp_systemConfig->getLocalizedText(L"entity:harvesterWaiting");
    return settings::gp_systemConfig->getLocalizedText(L"entity:harvesterActive");
}

ox::core::CString<wchar_t> CMineralGatherEntity::getMiniStatString()
{
    return settings::gp_systemConfig->getLocalizedText(L"entity:harvesterMiniStat", MineralsMined);
}

void CMineralGatherEntity::writeEntityData(ox::io::IWriteFile* file)
{
    ox::io::CHelpIO::writeInt(file, Target.Id);
    ox::io::CHelpIO::writeFloat(file, MiningTime);
    ox::io::CHelpIO::writeInt(file, Active);
    ox::io::CHelpIO::writeInt(file, MineralsMined);
}

void CMineralGatherEntity::readEntityData(ox::io::IReadFile* file, int version)
{
    Target.Id = ox::io::CHelpIO::readInt(file);
    MiningTime = ox::io::CHelpIO::readFloat(file);
    Active = ox::io::CHelpIO::readInt(file) != 0;
    if (version >= 12)
        MineralsMined = ox::io::CHelpIO::readInt(file);
}

} // end namespace entity
} // end namespace harvest
