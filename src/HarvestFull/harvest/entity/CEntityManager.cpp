// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include <math.h>
#include "ox/io/CHelpIO.h"
#include "harvest/game/CWorld.h"
#include "harvest/entity/CEntityManager.h"
#include "harvest/entity/CHarvestEntity.h"
#include "harvest/entity/CAlienEntity.h"
#include "harvest/entity/CSparkProducerEntity.h"
#include "harvest/entity/CSparkMoverEntity.h"
#include "harvest/entity/CMinerEntity.h"
#include "harvest/entity/CDefenseTowerEntity.h"
#include "harvest/entity/CMissileTurretEntity.h"
#include "harvest/entity/CMineralsEntity.h"
#include "ox/entity/ITestEntityFunction.h"
#include "ox/entity/ITestBestEntityFunction.h"
#include "ox/core/CBasic.h"
#include "ox/core/CMath.h"
#include "ox/algo/CRand.h"

namespace harvest {
namespace entity {

CEntityManager* gp_entityManager = 0;

// Names are ours; native indexed tables determine the eligible types.
static const bool COUNTED_BUILDINGS[21] =
{ true, true, false, false, true, false, false, true, true, false, false, false, false, true, true, false, true, false, false, false, false };
static const bool PLACEMENT_OBSTACLES[21] =
{ true, true, false, true, true, false, false, true, true, false, false, true, false, true, true, false, true, false, false, false, false };

class CFindClickableBuilding : public ox::entity::ITestBestEntityFunction
{
public:
    virtual int testEntity(ox::entity::COxEntity* entity, ox::entity::COxEntity* best);
    ox::core::CPosition2d<float> Position;
    float Distance;
};

class CFindRandomEntityInRange : public ox::entity::ITestEntityFunction
{
public:
    virtual bool testEntity(ox::entity::COxEntity* entity);
    ox::core::CPosition2d<float> Position;
    float SquaredRange;
    int EntityType;
    float Distance;
};

class CFindShootableAlienInRange : public ox::entity::ITestEntityFunction
{
public:
    virtual bool testEntity(ox::entity::COxEntity* entity);
    ox::core::CPosition2d<float> Position;
    float SquaredRange;
    float MinimumSquaredRange;
};

class CFindBuildingInRange : public ox::entity::ITestEntityFunction
{
public:
    virtual bool testEntity(ox::entity::COxEntity* entity);
    ox::core::CPosition2d<float> Position;
    float SquaredRange;
};

class CFindRangeLineBuildings : public ox::entity::ITestEntityFunction
{
public:
    virtual bool testEntity(ox::entity::COxEntity* entity);
    ox::core::CPosition2d<float> Position;
    int EntityType;
};

const ox::core::CRect<float>& CEntityManager::getBuildingsBoundingBox() const { return BuildingsBoundingBox; }

CEntityManager::CEntityManager()
    : COxEntityManager(5), ClickableSearch(0), RandomSearch(0), ShootableSearch(0), BuildingSearch(0),
      RangeLineSearch(0), BuildingsChanged(true), NumBuildings(0), NumEntities(0)
{
}

CEntityManager::~CEntityManager()
{
    delete ClickableSearch;
    delete RandomSearch;
    delete BuildingSearch;
    delete RangeLineSearch;
    delete ShootableSearch;
}

void CEntityManager::update(float frameDelta, const ox::core::CRect<float>& visibleArea)
{
    bool changed = !PendingEntities[0].empty() || NumEntities != EntityLists[0].size();
    updateAllEntities(frameDelta, visibleArea);
    BuildingsChanged = changed || ListChanged[0];
    if (BuildingsChanged)
    {
        NumBuildings = 0;
        BuildingsBoundingBox = ox::core::CRect<float>(300, 300, 724, 724);
        for (std::list<ox::entity::COxEntity*>::iterator it = EntityLists[0].begin(); it != EntityLists[0].end(); ++it)
        {
            if (COUNTED_BUILDINGS[(*it)->getEntityType()])
            {
                ++NumBuildings;
                ox::core::CVector3d<float> position = (*it)->getPosition();
                BuildingsBoundingBox.UpperLeftCorner.X = ox::core::min_(BuildingsBoundingBox.UpperLeftCorner.X, position.X);
                BuildingsBoundingBox.UpperLeftCorner.Y = ox::core::min_(BuildingsBoundingBox.UpperLeftCorner.Y, position.Y);
                BuildingsBoundingBox.LowerRightCorner.X = ox::core::max_(BuildingsBoundingBox.LowerRightCorner.X, position.X);
                BuildingsBoundingBox.LowerRightCorner.Y = ox::core::max_(BuildingsBoundingBox.LowerRightCorner.Y, position.Y);
            }
        }
        NumEntities = EntityLists[0].size();
    }
}

bool CEntityManager::readEntities(ox::io::IReadFile* file, int version)
{
    if (version < 6) return false;
    for (int layer = 0; layer < 5; ++layer)
    {
        if (layer == 4) continue;
        int count = ox::io::CHelpIO::readInt(file);
        for (int i = 0; i < count; ++i)
        {
            CEntity* entity = CEntity::readNextEntity(file, version);
            if (entity) appendEntity(entity, layer);
        }
    }
    return true;
}

bool CEntityManager::writeEntities(ox::io::IWriteFile* file)
{
    for (int layer = 0; layer < 5; ++layer)
    {
        if (layer == 4) continue;
        const std::list<ox::entity::COxEntity*>& entities = getEntityList(layer);
        std::list<ox::entity::COxEntity*>::const_iterator it = entities.begin();
        ox::io::CHelpIO::writeInt(file, entities.size());
        for (; it != entities.end(); ++it)
            ((CEntity*)*it)->writeEntity(file);
    }
    return true;
}
bool CEntityManager::hasBuildingListChanged() const { return BuildingsChanged || ListChanged[0]; }
int CEntityManager::getNumBuildings() const { return NumBuildings; }
int CEntityManager::getNumAliens() const { return EntityLists[1].size(); }

void CEntityManager::renderEntities(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort)
{
    const ox::TArray<ox::entity::COxEntity*>& entities = getRenderList();
    for (unsigned int i = 0; i < entities.size(); ++i)
        ((CEntity*)entities[i])->renderGroundLayer(camera, viewPort);
    for (unsigned int i = 0; i < entities.size(); ++i)
        ((CEntity*)entities[i])->render(camera, viewPort);
    for (unsigned int i = 0; i < TopLevelEnergyBeams.size(); ++i)
        renderEnergyBeam(TopLevelEnergyBeams[i], camera, viewPort);
    TopLevelEnergyBeams.clear();
}

void CEntityManager::renderEnergyBeam(SEnergyBeam* beam, const ox::core::CPosition2d<float>& camera,
    const ox::core::CRect<int>& viewPort)
{
    if (!beam || !beam->Beam) return;
    ox::core::CPosition2d<float> offset(viewPort.UpperLeftCorner.X - camera.X, viewPort.UpperLeftCorner.Y - camera.Y);
    ox::core::CPosition2d<float> start = beam->Start + offset;
    ox::core::CPosition2d<float> end = beam->End + offset;
    float angle = ox::core::CMath::getAngleIY(start, end) + 1.57079637f;
    float x = cos((double)angle) * beam->Width;
    float y = sin((double)angle) * beam->Width * .75;
    beam->Beam->drawFreeShape(
        ox::core::CPosition2d<float>(start.X + x, start.Y + y),
        ox::core::CPosition2d<float>(end.X + x, end.Y + y),
        ox::core::CPosition2d<float>(start.X - x, start.Y - y),
        ox::core::CPosition2d<float>(end.X - x, end.Y - y), beam->Color);
    if (beam->StartSprite) beam->StartSprite->drawScaled(start, beam->StartScale, beam->Color);
    if (beam->EndSprite) beam->EndSprite->drawScaled(end, beam->EndScale, beam->Color);
}

void CEntityManager::insertTopLevelEnergyBeam(SEnergyBeam* beam) { TopLevelEnergyBeams.push_back(beam); }

CEntity* CEntityManager::findClickableEntity(const ox::core::CPosition2d<float>& position)
{
    if (!ClickableSearch) ClickableSearch = new CFindClickableBuilding();
    ClickableSearch->Position = position;
    ClickableSearch->Distance = -1;
    return (CEntity*)findBestEntity(0, ClickableSearch);
}

bool CEntityManager::isBuildingPlacementOk(ox::core::CPosition2d<float>& position, float collisionSize)
{
    int attempts = 0;
    bool placementOk;
    do
    {
        ++attempts;
        placementOk = true;
        for (std::list<ox::entity::COxEntity*>::iterator it = EntityLists[0].begin();
            it != EntityLists[0].end(); ++it)
        {
            CEntity* entity = (CEntity*)*it;
            if (!PLACEMENT_OBSTACLES[entity->getEntityType()] && entity->getEntityType() != 5) continue;
            float radius = entity->getCollisionSize() + collisionSize;
            const ox::core::CVector3d<float>& entityCoordinates = entity->getPosition();
            ox::core::CPosition2d<float> entityPosition(entityCoordinates.X, entityCoordinates.Y);
            if (radius * radius > ox::core::CMath::getSquaredDistance(position, entityPosition))
            {
                const ox::core::CVector3d<float>& currentCoordinates = entity->getPosition();
                float angle = ox::core::CMath::getAngleIY(
                    ox::core::CPosition2d<float>(currentCoordinates.X, currentCoordinates.Y), position);
                position.X = entity->getPosition().X + cos((double)angle) * (radius + 1.0f);
                position.Y = entity->getPosition().Y + sin((double)angle) * (radius + 1.0f);
                placementOk = false;
                break;
            }
        }
    }
    while (!placementOk && attempts <= 2 && game::gp_world->mayPlaceObjectHere(position, false));
    return placementOk;
}

CEntity* CEntityManager::findRandomEntityInRange(const ox::core::CPosition2d<float>& position,
    float squaredRange, int layer, int entityType)
{
    if (!RandomSearch) RandomSearch = new CFindRandomEntityInRange();
    RandomSearch->Position = position;
    RandomSearch->SquaredRange = squaredRange;
    RandomSearch->EntityType = entityType;
    RandomSearch->Distance = 1e9f;
    ox::TArray<ox::entity::COxEntity*> entities;
    if (layer == 0 || layer == 1) findAllGridEntitiesInRange(entities, layer, RandomSearch);
    else findAllEntities(entities, layer, RandomSearch);
    if (entities.empty()) return 0;
    return (CEntity*)entities[ox::algo::CRand::rand() % entities.size()];
}

void CEntityManager::findAllGridEntitiesInRange(ox::TArray<ox::entity::COxEntity*>& result,
    int layer, CFindRandomEntityInRange* test)
{
    float radius = sqrtf(test->SquaredRange);
    int minX = calculateGridCoordinateClamp(test->Position.X - radius);
    int maxX = calculateGridCoordinateClamp(test->Position.X + radius);
    int minY = calculateGridCoordinateClamp(test->Position.Y - radius);
    int maxY = calculateGridCoordinateClamp(test->Position.Y + radius);
    for (int x = minX; x <= maxX; ++x)
        for (int y = minY; y <= maxY; ++y)
        {
            std::list<CEntity*>& cell = Grid[layer][y * 18 + x];
            for (std::list<CEntity*>::iterator it = cell.begin(); it != cell.end(); ++it)
                if (test->testEntity(*it)) result.push_back(*it);
        }
}

int CEntityManager::calculateGridCoordinateClamp(float coordinate)
{
    int cell = (int)((coordinate + game::WORLD_GRID_OFFSET) * .001953125f);
    cell = cell < 0 ? 0 : cell;
    cell = cell > 17 ? 17 : cell;
    return cell;
}

void CEntityManager::getAllEntitiesInRange(ox::TArray<ox::entity::COxEntity*>& result,
    const ox::core::CPosition2d<float>& position, float squaredRange, int layer, int entityType)
{
    if (!RandomSearch) RandomSearch = new CFindRandomEntityInRange();
    RandomSearch->Position = position;
    RandomSearch->SquaredRange = squaredRange;
    RandomSearch->EntityType = entityType;
    RandomSearch->Distance = 1e9f;
    findAllEntities(result, layer, RandomSearch);
}

CEntity* CEntityManager::findRandomAlienInRange(const ox::core::CPosition2d<float>& position,
    float squaredRange, float minimumSquaredRange)
{
    if (!ShootableSearch) ShootableSearch = new CFindShootableAlienInRange();
    ShootableSearch->Position = position;
    ShootableSearch->SquaredRange = squaredRange;
    ShootableSearch->MinimumSquaredRange = minimumSquaredRange;
    ox::TArray<ox::entity::COxEntity*> entities;
    findAllEntities(entities, 1, ShootableSearch);
    if (entities.empty()) return 0;
    return (CEntity*)entities[ox::algo::CRand::rand() % entities.size()];
}

CEntity* CEntityManager::findAnyEntityInRange(const ox::core::CPosition2d<float>& position,
    float squaredRange, int layer, int entityType)
{
    if (!RandomSearch) RandomSearch = new CFindRandomEntityInRange();
    RandomSearch->Position = position;
    RandomSearch->SquaredRange = squaredRange;
    RandomSearch->EntityType = entityType;
    RandomSearch->Distance = 1e9f;
    return (CEntity*)findFirstEntity(layer, RandomSearch);
}

CEntity* CEntityManager::findBuildingInRange(const ox::core::CPosition2d<float>& position, float squaredRange)
{
    if (!BuildingSearch) BuildingSearch = new CFindBuildingInRange();
    BuildingSearch->Position = position;
    BuildingSearch->SquaredRange = squaredRange;
    return (CEntity*)findFirstEntity(0, BuildingSearch);
}

float CEntityManager::getRangeSearchResultDistance() { return RandomSearch ? RandomSearch->Distance : 1e9f; }

void CEntityManager::getAllRangeLineBuildings(ox::TArray<ox::entity::COxEntity*>& result,
    const ox::core::CPosition2d<float>& position, int entityType)
{
    if (!RangeLineSearch) RangeLineSearch = new CFindRangeLineBuildings();
    RangeLineSearch->Position = position;
    RangeLineSearch->EntityType = entityType;
    findAllEntities(result, 0, RangeLineSearch);
}

CEntity* CEntityManager::updateClickableReference(int id, CEntity* entity)
{
    if (entity && ListChanged[0]) return (CEntity*)locateEntity(id, 0);
    return entity;
}

CEntity* CEntityManager::addBuilding(int type, float x, float y)
{
    CEntity* entity = 0;
    switch (type)
    {
    case 0: entity = new CSparkProducerEntity(x, y); break;
    case 1: entity = new CSparkMoverEntity(x, y); break;
    case 4: entity = new CMineralGatherEntity(x, y); break;
    case 7: entity = new CDefenseTowerEntity(x, y); break;
    case 8: entity = new CMissileTurretEntity(8, x, y); break;
    }
    if (entity) appendEntity(entity, 0);
    return entity;
}

CMineralsEntity* CEntityManager::addMinerals(int size, float x, float y)
{
    CMineralsEntity* entity = new CMineralsEntity(x, y, size);
    appendEntity(entity, 0);
    return entity;
}

CAlienEntity* CEntityManager::addAlien(int type, float x, float y)
{
    CAlienEntity* entity = new CAlienEntity(x, y, type);
    appendEntity(entity, 1);
    return entity;
}

void CEntityManager::addGridEntity(CEntity* entity, int searchLayer)
{
    int cell = calculateGridPosition(entity->getPosition());
    if (cell >= 0) Grid[searchLayer][cell].push_back(entity);
}

int CEntityManager::calculateGridPosition(const ox::core::CVector3d<float>& position)
{
    int x = (int)((position.X + game::WORLD_GRID_OFFSET) * .001953125f);
    int y = (int)((position.Y + game::WORLD_GRID_OFFSET) * .001953125f);
    if (x < 0 || x > 17 || y < 0 || y > 17) return -1;
    return y * 18 + x;
}

void CEntityManager::updateGridEntity(CEntity* entity, const ox::core::CVector3d<float>& oldPosition, int searchLayer)
{
    int cell = calculateGridPosition(entity->getPosition());
    int oldCell = calculateGridPosition(oldPosition);
    if (cell != oldCell)
    {
        if (oldCell >= 0)
        {
            std::list<CEntity*>& previous = Grid[searchLayer][oldCell];
            for (std::list<CEntity*>::iterator it = previous.begin(); it != previous.end(); ++it)
                if (*it == entity) { previous.erase(it); break; }
        }
        if (cell >= 0) Grid[searchLayer][cell].push_back(entity);
    }
}

void CEntityManager::removeGridEntity(CEntity* entity, const ox::core::CVector3d<float>& position, int searchLayer)
{
    int cell = calculateGridPosition(position);
    if (cell >= 0)
    {
        std::list<CEntity*>& entries = Grid[searchLayer][cell];
        for (std::list<CEntity*>::iterator it = entries.begin(); it != entries.end(); ++it)
            if (*it == entity) { entries.erase(it); break; }
    }
}

CEntity* CEntityManager::locateEntityInGrid(int id, int gridPosition, int layer)
{
    std::list<CEntity*>& entries = Grid[layer][gridPosition];
    for (std::list<CEntity*>::iterator it = entries.begin(); it != entries.end(); ++it)
        if ((*it)->getId() == id && !(*it)->isKilled()) return *it;
    return 0;
}

int CFindClickableBuilding::testEntity(ox::entity::COxEntity* entity, ox::entity::COxEntity* best)
{
    float distance = ox::core::CMath::getSquaredDistance(
        ox::core::CPosition2d<float>(entity->getPosition().X, entity->getPosition().Y), Position);
    if (distance <= 625.0f && (Distance < 0 || distance < Distance))
    {
        Distance = distance;
        return true;
    }
    return false;
}

bool CFindRandomEntityInRange::testEntity(ox::entity::COxEntity* entity)
{
    int entityType = EntityType;
    if (entityType >= 0)
    {
        if (entity->getEntityType() != entityType) return false;
    }
    float distance = ox::core::CMath::getSquaredDistance(
        ox::core::CPosition2d<float>(entity->getPosition().X, entity->getPosition().Y), Position);
    if (distance <= SquaredRange) return true;
    if (distance < Distance) Distance = distance;
    return false;
}

bool CFindShootableAlienInRange::testEntity(ox::entity::COxEntity* entity)
{
    if (entity->getEntityType() != 6) return false;
    if (((CAlienEntity*)entity)->getAlienType() == 8 && entity->getPosition().Z > 10.0f) return false;
    float distance = ox::core::CMath::getSquaredDistance(
        ox::core::CPosition2d<float>(entity->getPosition().X, entity->getPosition().Y), Position);
    return distance <= SquaredRange && distance >= MinimumSquaredRange;
}

bool CFindBuildingInRange::testEntity(ox::entity::COxEntity* entity)
{
    if (entity->getEntityType() == 5) return false;
    return ox::core::CMath::getSquaredDistance(
        ox::core::CPosition2d<float>(entity->getPosition().X, entity->getPosition().Y), Position) <= SquaredRange;
}

bool CFindRangeLineBuildings::testEntity(ox::entity::COxEntity* entity)
{
    int type = entity->getEntityType();
    int entityType = EntityType;
    switch (type)
    {
    case 3:
        if (entityType != 1) return false;
        break;
    case 5:
        if (entityType != 4) return false;
        break;
    case 1:
        break;
    default:
        return false;
    }
    float distance = ox::core::CMath::getSquaredDistance(
        ox::core::CPosition2d<float>(entity->getPosition().X, entity->getPosition().Y), Position);
    if (type == 5)
        return distance <= 10000.0f;
    return distance <= 22500.0f;
}

} // end namespace entity
} // end namespace harvest
