// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

// The object has an iostream static initializer, set up before the game headers' statics; the
// header that pulled it in is not identified yet. The world grid offset is initialized before
// ENERGY_PROGRESS_COLOR, so CWorld.h comes first.
#include <iostream>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CConstructionEntity.h"
#include "harvest/ECustomEvents.h"
#include "harvest/entity/CBuildableItems.h"
#include "harvest/entity/CCreativeEntity.h"
#include "harvest/entity/CDefenseTowerEntity.h"
#include "harvest/entity/CEntityManager.h"
#include "harvest/entity/CMinerEntity.h"
#include "harvest/entity/CMissileTurretEntity.h"
#include "harvest/entity/CSparkMoverEntity.h"
#include "harvest/entity/CSparkProducerEntity.h"
#include "harvest/game/CLuaManager.h"
#include "harvest/game/CStatistics.h"
#include "harvest/settings/CSystemConfig.h"
#include "ox/core/CMath.h"
#include "ox/event/IEventReceiver.h"
#include "ox/io/CHelpIO.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {
namespace entity {

CConstructionEntity::CConstructionEntity(float x, float y, const char* buildingId)
    : CBuildingEntity(g_nextEntityId++, 3, x, y), Sparks(0), MoversCalled(false), Sprite(0)
{
    if (gp_buildableItems && buildingId)
    {
        BuildingId = buildingId;
        setEntityInfo();
    }
}

void CConstructionEntity::setEntityInfo()
{
    SparksNeeded = -1;
    if (!gp_buildableItems)
        return;

    SBuildingInfoItem* info = gp_buildableItems->getBuildingInfo(
        gp_buildableItems->getIndexForEntityId(BuildingId.c_str()));
    if (!info)
        return;

    EntityType = info->EntityType;
    SparksNeeded = info->SparkCost.getValue();
    CollisionSize = info->CollisionSize;
    MineralCost = info->MineralCost.getValue();
    Name = info->Name;

    if (EntityType == 16)
        setSprite(info->SpritePackage.c_str(), info->SpriteName.c_str());
    else
        setSprite(0, info->SpriteName.c_str());
}

CConstructionEntity::~CConstructionEntity()
{
    if (Sprite)
        Sprite->remove();
}

void CConstructionEntity::setSprite(const char* packageFile, const char* spriteName)
{
    if (Sprite)
    {
        Sprite->remove();
        Sprite = 0;
    }

    ox::video::ISpritePackage* package = gp_spritePackage;
    if (packageFile)
        package = gp_videoDriver->getSpritePackage(packageFile, false);

    if (package)
        Sprite = package->addNewAnimationState(ox::core::CString<char>(spriteName));
}

const char* CConstructionEntity::getBuildingId()
{
    return BuildingId.c_str();
}

void CConstructionEntity::updateSprite(float frameDelta)
{
    if (Sprite)
        Sprite->update(frameDelta);
}

int CConstructionEntity::updateLogic(float frameDelta)
{
    if (SparksNeeded <= 0)
        return 1;
    if (Sparks < SparksNeeded)
        return 0;

    CBuildingEntity* building = 0;
    switch (EntityType)
    {
    case 0:
        building = new CSparkProducerEntity(Position.X, Position.Y);
        if (game::gp_world->getGameMode() == 0)
        {
            ox::event::SEvent event;
            event.EventType = ox::event::EET_USER_EVENT;
            event.UserEvent.UserData1 = ECE_TUTORIAL_HINT;
            // the hint for a completed solar collector
            event.UserEvent.UserData2 = 28;
            event.UserEvent.UserData3 = 0;
            event.UserEvent.UserPointer = 0;
            ox::event::gp_subscriberList->OnEvent(event);
        }
        break;
    case 1:
        building = new CSparkMoverEntity(Position.X, Position.Y);
        break;
    case 4:
        building = new CMineralGatherEntity(Position.X, Position.Y);
        break;
    case 7:
        building = new CDefenseTowerEntity(Position.X, Position.Y);
        break;
    case 8:
    case 13:
    case 14:
        building = new CMissileTurretEntity(EntityType, Position.X, Position.Y);
        break;
    case 16:
        building = new CCreativeEntity(Position.X, Position.Y, BuildingId.c_str());
        break;
    }

    if (building)
    {
        building->setId(Id);
        gp_entityManager->appendEntity(building, 0);
        killEntity();
        if (game::gp_luaManager)
            game::gp_luaManager->hookBuildingConstructed(building);
    }

    if (game::gp_statistics)
        game::gp_statistics->modifyGameStatValue(3, 1);
    return 1;
}

ox::core::CString<wchar_t> CConstructionEntity::getInfoString()
{
    return settings::gp_systemConfig->getLocalizedText(L"entity:construction", Name.c_str(),
        (int)((float)Sparks / SparksNeeded * 100.0f));
}

void CConstructionEntity::handleDoubleClickSelection()
{
    if (!MoversCalled)
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
                    {
                        mover->setSparkTargetId(Id, false);
                        MoversCalled = true;
                    }
                }
            }
        }
    }
    else
    {
        MoversCalled = false;
        const std::list<ox::entity::COxEntity*>& entities = gp_entityManager->getEntityList(0);
        for (std::list<ox::entity::COxEntity*>::const_iterator it = entities.begin(); it != entities.end(); ++it)
        {
            if ((*it)->getEntityType() == 1)
            {
                CSparkMoverEntity* mover = (CSparkMoverEntity*)*it;
                if (mover->getWaypointId() == Id)
                    mover->handleDoubleClickSelection();
            }
        }
    }
}

int CConstructionEntity::onSpark(CSparkEntity* spark)
{
    if (Sparks >= SparksNeeded)
        return -1;
    ++Sparks;
    return 0;
}

bool CConstructionEntity::wantsSpark()
{
    return Sparks < SparksNeeded;
}

void CConstructionEntity::render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort)
{
    CEntity::renderSpriteFixed(camera, viewPort, Sprite);
    if (SparksNeeded > 0)
        renderSelfProgress(camera, viewPort, (float)Sparks / SparksNeeded, ENERGY_PROGRESS_COLOR);
}

ox::video::ISpriteAnimationState* CConstructionEntity::getCurrentDisplaySprite()
{
    return Sprite;
}

void CConstructionEntity::writeEntityData(ox::io::IWriteFile* file)
{
    ox::io::CHelpIO::writeInt(file, EntityType);
    ox::io::CHelpIO::writeInt(file, Sparks);
    ox::io::CHelpIO::writeInt(file, SparksNeeded);
    ox::io::CHelpIO::writeString(file, BuildingId);
}

void CConstructionEntity::readEntityData(ox::io::IReadFile* file, int version)
{
    EntityType = ox::io::CHelpIO::readInt(file);
    Sparks = ox::io::CHelpIO::readInt(file);
    SparksNeeded = ox::io::CHelpIO::readInt(file);

    if (version < 17)
    {
        SBuildingInfoItem* info = gp_buildableItems->getBuildingInfo(
            gp_buildableItems->getIndexForEntityType(EntityType));
        if (info)
            BuildingId = info->EntityId;
    }
    else
        ox::io::CHelpIO::readString(file, BuildingId);

    setEntityInfo();
}

} // end namespace entity
} // end namespace harvest
