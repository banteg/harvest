// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

// The object has an iostream static initializer, set up before the game headers' statics; the
// header that pulled it in is not identified yet. The world grid offset is initialized before
// ENERGY_PROGRESS_COLOR, so CWorld.h comes first.
#include <iostream>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CSparkMoverEntity.h"
#include "harvest/ECustomEvents.h"
#include "harvest/entity/CPerimeterBomb.h"
#include "harvest/entity/CSparkEntity.h"
#include "harvest/game/CLuaManager.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/core/CBasic.h"
#include "ox/core/CMath.h"
#include "ox/event/IEventReceiver.h"
#include "ox/io/CHelpIO.h"
#include "ox/video/IParticlePackage.h"
#include "ox/video/IParticleState.h"
#include "ox/video/ISpritePackage.h"

namespace harvest {
namespace entity {

bool g_useLargeSparkDeathParticle = false;

CSparkMoverEntity::CSparkMoverEntity(float x, float y)
    : CBuildingEntity(g_nextEntityId++, 1, x, y), SparkIndex(0), AlienWaypoint(false),
      WaypointToConstruction(false), Heat(0), Cooldown(0), LinkBeam(4.0f), AlienBeam(11.0f), Charging(false),
      ChargeParticle(0), HeatSampleTimer(0), PeakHeat(0)
{
    Sprite = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("SparkMover"));
    LinkBeam.Beam = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("EnergyRedirect"));
    AlienBeam.Beam = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("Stealer"));
    LinkBeam.Start = ox::core::CPosition2d<float>(Position.X, Position.Y);
    AlienBeam.Start = ox::core::CPosition2d<float>(Position.X, Position.Y - 4.0f);
}

CSparkMoverEntity::~CSparkMoverEntity()
{
    if (Sprite)
        Sprite->remove();
    if (ChargeParticle)
        ChargeParticle->remove();
}

void CSparkMoverEntity::renderGroundLayer(const ox::core::CPosition2d<float>& camera,
    const ox::core::CRect<int>& viewPort)
{
    if (!isWaypointed() || AlienWaypoint)
        return;

    LinkBeam.End = ox::core::CPosition2d<float>(WaypointPosition.X, WaypointPosition.Y);
    gp_entityManager->renderEnergyBeam(&LinkBeam, camera, viewPort);
}

void CSparkMoverEntity::render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort)
{
    if (isAlienWaypointed() && Waypoint.Entity)
    {
        AlienBeam.End = ox::core::CPosition2d<float>(WaypointPosition.X, WaypointPosition.Y - 25.0f);
        gp_entityManager->insertTopLevelEnergyBeam(&AlienBeam);
    }

    CEntity::renderSpriteFixed(camera, viewPort, Sprite);

    if (Charging)
        renderSelfProgress(camera, viewPort, ChargeSparks / 30.0f, ENERGY_PROGRESS_COLOR);
}

ox::video::ISpriteAnimationState* CSparkMoverEntity::getCurrentDisplaySprite()
{
    return Sprite;
}

int CSparkMoverEntity::updateLogic(float frameDelta)
{
    Heat -= frameDelta;

    // cool links are grey, hot ones red
    if (Charging)
        Color = ox::video::SColor(0xffffffff);
    else if (Heat < 0)
    {
        if (Heat < -0.6f)
            Color = ox::video::SColor(0xffcccccc);
        else
        {
            int shade = (int)((Heat + 0.6f) / 0.6f * 51.0f) + 204;
            Color = ox::video::SColor(255, shade, shade, shade);
        }
    }
    else if (Heat > 0)
    {
        int shade = (int)((1.0f - Heat) * 255.0f);
        if (shade > 255)
            shade = 255;
        else if (shade < 64)
            shade = 64;
        Color = ox::video::SColor(255, 255, shade, shade);
    }
    else
        Color = ox::video::SColor(0xffffffff);

    if (isWaypointed())
    {
        if (Cooldown > 0)
            Cooldown -= frameDelta;

        updateTargetReference();
        if (!Waypoint.Entity)
            Waypoint.Id = 0;
        else if (isAlienWaypointed() || Waypoint.Entity->getEntityType() == 1 || Waypoint.Entity->getEntityType() == 3)
            WaypointPosition = Waypoint.Entity->getPosition();
        else
        {
            Waypoint.Entity = 0;
            Waypoint.Id = 0;
        }
    }

    updateSparkTargets(SparkTargets);

    if (Charging && ChargeParticle)
    {
        ox::core::CVector3d<float> position(Position.X, Position.Y + 1.0f, Position.Z);
        ChargeParticle->update((ChargeSparks / 30.0f * 3.0f + 1.0f) * frameDelta, position);
    }

    HeatSampleTimer -= frameDelta;
    if (HeatSampleTimer <= 0)
    {
        HeatSampleTimer = 1.0f;
        PeakHeat = Heat;
    }

    return 0;
}

void CSparkMoverEntity::handleSelectionDraggedToNothing()
{
    if (isAlienWaypointed())
        return;
    Waypoint.Id = -1;
    Waypoint.Entity = 0;
}

bool CSparkMoverEntity::isWaypointed()
{
    return Waypoint.Id > 0;
}

void CSparkMoverEntity::updateTargetReference()
{
    if (AlienWaypoint)
    {
        gp_entityManager->updateReference(Waypoint, 1, true);
        return;
    }

    gp_entityManager->updateReference(Waypoint, 0, true);
    if (!Waypoint.Entity)
        WaypointToConstruction = false;
    else if (Waypoint.Entity->getEntityType() == 3)
        WaypointToConstruction = true;
    else
    {
        if (WaypointToConstruction)
        {
            Waypoint.Entity = 0;
            Waypoint.Id = -1;
        }
        WaypointToConstruction = false;
    }
}

bool CSparkMoverEntity::isAlienWaypointed()
{
    return isWaypointed() && AlienWaypoint;
}

void CSparkMoverEntity::updateSprite(float frameDelta)
{
    if (Sprite)
    {
        if (Charging)
            Sprite->update((ChargeSparks / 30.0f * 3.0f + 1.0f) * frameDelta);
        else
            Sprite->update(frameDelta);
    }
    if (LinkBeam.Beam)
        LinkBeam.Beam->update(frameDelta);
    if (AlienBeam.Beam)
        AlienBeam.Beam->update(frameDelta);
}

int CSparkMoverEntity::onSpark(CSparkEntity* spark)
{
    if (Charging)
    {
        if (ChargeSparks >= 30)
            return -1;

        if (++ChargeSparks == 30)
        {
            Killed = true;
            gp_entityManager->appendEntity(new CPerimeterBombExplosion(Position.X, Position.Y), 3);
            gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y, 1.0f, 0, "OverchargeDone"), 4);

            ox::event::SEvent event;
            event.EventType = ox::event::EET_USER_EVENT;
            event.UserEvent.UserData1 = ECE_TUTORIAL_HINT;
            // the hint for a finished overcharge
            event.UserEvent.UserData2 = 29;
            event.UserEvent.UserData3 = 0;
            event.UserEvent.UserPointer = 0;
            ox::event::gp_subscriberList->OnEvent(event);

            if (game::gp_luaManager)
                game::gp_luaManager->hookEnergyLinkCharged(Position.X, Position.Y);
        }
        return 0;
    }

    if (Heat < 0)
        Heat = 0;

    if (Heat <= 1.0f && !isAlienWaypointed())
    {
        Heat += 1.0f / 30.0f;
        PeakHeat = ox::core::max_(PeakHeat, Heat);

        if (isWaypointed())
        {
            updateTargetReference();
            if (Waypoint.Entity && (Waypoint.Entity->getEntityType() == 3 || Cooldown > 0
                    || (Waypoint.Entity->getEntityType() == 1 && ((CSparkMoverEntity*)Waypoint.Entity)->Charging)))
                return Waypoint.Id;

            updateSparkTargets(SparkTargets);
            int target = selectRequiredSparkTarget();
            if (target > 0)
            {
                Cooldown += 0.25f;
                return target;
            }
            if (Waypoint.Entity)
                return Waypoint.Id;
        }

        updateSparkTargets(SparkTargets);
        int target = selectSparkTarget(SparkTargets, spark->getSourceId(), SparkIndex);
        if (target > 0)
            return target;
    }
    else
        Heat = ox::core::max_(Heat - 1.0f / 60.0f, 0.0f);

    // the spark dies here
    gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y, 1.0f, 0,
        g_useLargeSparkDeathParticle ? "SparkDeath" : "Poof"), 4);
    if (game::gp_luaManager && !isAlienWaypointed())
        game::gp_luaManager->hookEnergyLinkOverheated(this);
    return 0;
}

int CSparkMoverEntity::selectRequiredSparkTarget()
{
    if (SparkTargets.empty())
        return 0;

    int start = SparkIndex % SparkTargets.size();
    CEntity* entity = (CEntity*)SparkTargets[start];
    ++SparkIndex;

    while (!(entity->wantsSpark() && (entity->getEntityType() != 1 || ((CSparkMoverEntity*)entity)->Charging)))
    {
        int current = SparkIndex % SparkTargets.size();
        entity = (CEntity*)SparkTargets[current];
        if (current == start)
        {
            entity = 0;
            break;
        }
        ++SparkIndex;
    }

    if (entity)
        return entity->getId();
    return 0;
}

bool CSparkMoverEntity::acceptsSparkFrom(int id)
{
    if (!Waypoint.Entity)
        return true;
    return Waypoint.Id != id;
}

bool CSparkMoverEntity::isOverheated()
{
    return Heat > 0.9f;
}

ox::core::CString<wchar_t> CSparkMoverEntity::getInfoString()
{
    if (Charging)
        return settings::gp_systemConfig->getLocalizedText(L"entity:energyCharging",
            (int)(ChargeSparks * 100.0f / 30.0f));
    return settings::gp_systemConfig->getLocalizedText(L"entity:energyForwarding");
}

ox::core::CString<wchar_t> CSparkMoverEntity::getMiniStatString()
{
    return settings::gp_systemConfig->getLocalizedText(L"entity:energyHeatLevel",
        PeakHeat > 0 ? (int)(PeakHeat * 100.0f) : 0);
}

void CSparkMoverEntity::handleRightClickAction(const ox::core::CPosition2d<float>& position)
{
    if (isAlienWaypointed())
        return;

    CEntity* entity = gp_entityManager->findClickableEntity(position);
    if (!entity)
        return;

    if (entity == this)
    {
        Waypoint.Id = -1;
        Waypoint.Entity = 0;
    }
    else
        handleSelectionDraggedToEntity(entity);
}

void CSparkMoverEntity::handleDoubleClickSelection()
{
    if (isAlienWaypointed())
        return;

    if (Charging)
    {
        const std::list<ox::entity::COxEntity*>& entities = gp_entityManager->getEntityList(0);
        for (std::list<ox::entity::COxEntity*>::const_iterator it = entities.begin(); it != entities.end(); ++it)
        {
            if ((*it)->getEntityType() == 1)
            {
                const ox::core::CVector3d<float>& position = (*it)->getPosition();
                if (ox::core::CMath::getSquaredDistance(ox::core::CPosition2d<float>(Position.X, Position.Y),
                        ox::core::CPosition2d<float>(position.X, position.Y)) <= 22500.0f)
                {
                    CSparkMoverEntity* mover = (CSparkMoverEntity*)*it;
                    if (!mover->isWaypointed())
                        mover->setSparkTargetId(Id, false);
                }
            }
        }
    }
    else if (isWaypointed())
        clearWaypointsForward();
}

void CSparkMoverEntity::setSparkTargetId(int id, bool alien)
{
    Waypoint.Id = id;
    AlienWaypoint = alien;
    Waypoint.Entity = 0;
}

void CSparkMoverEntity::clearWaypointsForward()
{
    if (isWaypointed() && !AlienWaypoint)
    {
        ox::entity::COxEntity* next = gp_entityManager->locateEntity(Waypoint.Id, 0);
        Waypoint.Id = -1;
        Waypoint.Entity = 0;
        if (next && next->getEntityType() == 1)
            ((CSparkMoverEntity*)next)->clearWaypointsForward();
    }
}

bool CSparkMoverEntity::handleSelectionDraggedToEntity(CEntity* entity)
{
    if (entity->getEntityType() == 3 || entity->getEntityType() == 1)
    {
        if (this != entity)
        {
            float dx = Position.X - entity->getPosition().X;
            float dy = Position.Y - entity->getPosition().Y;
            if (dx * dx + dy * dy <= 22500.0f)
            {
                setSparkTargetId(entity->getId(), false);
                return true;
            }
        }
    }
    return false;
}

int CSparkMoverEntity::getWaypointId()
{
    return Waypoint.Id;
}

void CSparkMoverEntity::startCharging()
{
    if (Charging)
        return;

    Charging = true;
    ChargeSparks = 0;
    if (!ChargeParticle)
        ChargeParticle = gp_particlePackage->addNewParticleState(ox::core::CString<char>("Overcharge"));

    if (game::gp_luaManager)
        game::gp_luaManager->hookEnergyLinkCharging(this);
}

void CSparkMoverEntity::writeEntityData(ox::io::IWriteFile* file)
{
    ox::io::CHelpIO::writeInt(file, SparkIndex);
    ox::io::CHelpIO::writeInt(file, Waypoint.Id);
    ox::io::CHelpIO::writeFloat(file, Heat);
    ox::io::CHelpIO::writeInt(file, AlienWaypoint);
    ox::io::CHelpIO::writeInt(file, Charging);
    ox::io::CHelpIO::writeInt(file, ChargeSparks);
    ox::io::CHelpIO::writeFloat(file, Cooldown);
}

void CSparkMoverEntity::readEntityData(ox::io::IReadFile* file, int version)
{
    SparkIndex = ox::io::CHelpIO::readInt(file);
    Waypoint.Id = ox::io::CHelpIO::readInt(file);
    Heat = ox::io::CHelpIO::readFloat(file);
    if (version >= 5)
    {
        AlienWaypoint = ox::io::CHelpIO::readInt(file) != 0;
        if (version >= 6)
        {
            Charging = ox::io::CHelpIO::readInt(file) != 0;
            ChargeSparks = ox::io::CHelpIO::readInt(file);
            Cooldown = ox::io::CHelpIO::readFloat(file);
        }
    }
}

} // end namespace entity
} // end namespace harvest
