// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

// The object has an iostream static initializer, set up before the game headers' statics; the
// header that pulled it in is not identified yet. The world grid offset is initialized before
// ENERGY_PROGRESS_COLOR, so CWorld.h comes first.
#include <iostream>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CHarvestEntity.h"
#include "harvest/entity/CAlienEntity.h"
#include "harvest/entity/CConstructionEntity.h"
#include "harvest/entity/CCreativeEntity.h"
#include "harvest/entity/CDefenseTowerEntity.h"
#include "harvest/entity/CEntityManager.h"
#include "harvest/entity/CMineralsEntity.h"
#include "harvest/entity/CMinerEntity.h"
#include "harvest/entity/CMissileTurretEntity.h"
#ifdef HARVEST_PORT
#include "harvest/entity/CPerimeterBomb.h"
#endif
#include "harvest/entity/CSparkEntity.h"
#include "harvest/entity/CSparkMoverEntity.h"
#include "harvest/entity/CSparkProducerEntity.h"
#include "ox/entity/ITestEntityFunction.h"
#include "ox/io/CHelpIO.h"
#include "ox/video/IParticlePackage.h"
#include "ox/video/IParticleState.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/IVideoDriver.h"

namespace harvest {
namespace entity {

//! The entity manager's search layer of each entity type.
static const int ENTITY_SEARCH_LAYERS[] =
{
    0, 0, 2, 0, 0, 0, 1, 0, 0, 3, 4, 3, 3, 0, 0, 3, 0, 4, 3, 3, 4
};

//! Accepts the other live entities within spark range, minerals excepted.
class CSparkListBuilder : public ox::entity::ITestEntityFunction
{
public:
    CSparkListBuilder(int id, const ox::core::CVector3d<float>& position)
        : Id(id), Position(position)
    {
    }

    virtual bool testEntity(ox::entity::COxEntity* entity)
    {
        if (entity->getId() == Id || entity->isKilled() || entity->getEntityType() == 5)
            return false;

        float dx = Position.X - entity->getPosition().X;
        float dy = Position.Y - entity->getPosition().Y;
        return dx * dx + dy * dy <= 22500.0f;
    }

private:
    int Id;
    ox::core::CVector3d<float> Position;
};

//! Accepts the other entities within spark range that want a spark.
class CFindSparkFunctor : public ox::entity::ITestEntityFunction
{
public:
    CFindSparkFunctor(int id, int excludeId, const ox::core::CVector3d<float>& position)
        : Id(id), ExcludeId(excludeId), Position(position)
    {
    }

    virtual bool testEntity(ox::entity::COxEntity* entity)
    {
        if (entity->getId() == Id || entity->getId() == ExcludeId)
            return false;
        if (!((CEntity*)entity)->wantsSpark())
            return false;

        float dx = Position.X - entity->getPosition().X;
        float dy = Position.Y - entity->getPosition().Y;
        return dx * dx + dy * dy <= 22500.0f;
    }

private:
    int Id;
    int ExcludeId;
    ox::core::CVector3d<float> Position;
};

ox::video::ISpritePackage* CEntity::gp_spritePackage = 0;
ox::video::IParticlePackage* CEntity::gp_particlePackage = 0;
ox::video::IVideoDriver* CEntity::gp_videoDriver = 0;
ox::audio::IAudioDriver* CEntity::gp_audioDriver = 0;
ox::gui::IGUIFont* CEntity::gp_alienChantFont = 0;
ox::core::CDimension2d<float> CEntity::g_screenSizeF;
ox::core::CPosition2d<float> CEntity::g_screenCenterPos;

CEntity::CEntity(int id, int type, float x, float y)
    : COxEntity(id, x, y, 0), Type(type), Color(0xffffffff)
{
    if (Id > 0 && Type != 2)
        gp_entityManager->addGridEntity(this, ENTITY_SEARCH_LAYERS[type]);
}

CEntity::~CEntity()
{
}

int CEntity::getEntityType()
{
    return Type;
}

int CEntity::update(float frameDelta)
{
    int done = updateLogic(frameDelta);
    updateSprite(frameDelta);
    return done;
}

void CEntity::notifyRemoved()
{
    if (Id > 0 && Type != 2)
        gp_entityManager->removeGridEntity(this, Position, ENTITY_SEARCH_LAYERS[Type]);
}

void CEntity::updateSprite(float frameDelta)
{
}

bool CEntity::addToRenderList(const ox::core::CRect<float>& visibleArea)
{
    float x = Position.X;
    float y = Position.Y;
    return x >= visibleArea.UpperLeftCorner.X && y >= visibleArea.UpperLeftCorner.Y
        && x < visibleArea.LowerRightCorner.X && y < visibleArea.LowerRightCorner.Y;
}

void CEntity::renderSprite(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort,
    ox::video::ISpriteAnimationState* sprite)
{
    if (!sprite)
        return;

    ox::core::CPosition2d<float> pos;
    pos.X = Position.X - camera.X + viewPort.UpperLeftCorner.X;
    pos.Y = Position.Y - camera.Y + viewPort.UpperLeftCorner.Y;
    sprite->drawScaled(pos, 1.0f, Color);
}

void CEntity::renderSpriteFixed(const ox::core::CPosition2d<float>& camera,
    const ox::core::CRect<int>& viewPort, ox::video::ISpriteAnimationState* sprite)
{
    if (!sprite)
        return;

    int x = (int)(Position.X - camera.X + viewPort.UpperLeftCorner.X);
    int y = (int)(Position.Y - camera.Y + viewPort.UpperLeftCorner.Y);
    sprite->draw(ox::core::CPosition2d<int>(x, y), 0, Color);
}

void CEntity::renderEnergyLine(const ox::core::CPosition2d<float>& camera,
    const ox::core::CRect<int>& viewPort, const ox::core::CPosition2d<float>& target, bool minerals)
{
    ox::video::SColor color;
    float range;

    if (Type == 1)
    {
        color = ox::video::SColor(0xff8080c0);
        range = 150.0f;
    }
    else if (Type == 5 && minerals)
    {
        color = ox::video::SColor(0xff80c080);
        range = 100.0f;
    }
    else
        return;

    float dx = Position.X - target.X;
    float dy = Position.Y - target.Y;
    if (dx * dx + dy * dy > range * range)
        return;

    gp_videoDriver->draw2DLine(
        ox::core::CPosition2d<int>((int)(Position.X - camera.X + viewPort.UpperLeftCorner.X),
            (int)(Position.Y - camera.Y + viewPort.UpperLeftCorner.Y)),
        ox::core::CPosition2d<int>((int)(target.X - camera.X + viewPort.UpperLeftCorner.X),
            (int)(target.Y - camera.Y + viewPort.UpperLeftCorner.Y)),
        color);
}

void CEntity::renderSelfProgress(const ox::core::CPosition2d<float>& camera,
    const ox::core::CRect<int>& viewPort, float progress, ox::video::SColor color)
{
    ox::core::CPosition2d<int> pos((int)(Position.X - camera.X + viewPort.UpperLeftCorner.X),
        (int)(Position.Y - camera.Y + viewPort.UpperLeftCorner.Y));

    ox::core::CRect<int> rect(pos.X - 11, pos.Y + 11, pos.X + 11, pos.Y + 16);
    gp_videoDriver->draw2DRectangle(ox::video::SColor(0xff000000), rect, &viewPort);

    if (progress > 0)
    {
        rect.UpperLeftCorner.X += 1;
        rect.UpperLeftCorner.Y += 1;
        rect.LowerRightCorner.X -= 1;
        rect.LowerRightCorner.Y -= 1;
        rect.LowerRightCorner.X = rect.UpperLeftCorner.X
            + (int)(rect.getWidth() * progress);
        gp_videoDriver->draw2DRectangle(color, rect, &viewPort);
    }
}

ox::video::ISpriteAnimationState* CEntity::getCurrentDisplaySprite()
{
    return 0;
}

int CEntity::findSparkTarget(int excludeId, int& index)
{
    int result = 0;

    if (gp_entityManager)
    {
        CFindSparkFunctor* test = new CFindSparkFunctor(Id, excludeId, Position);
        ox::TArray<ox::entity::COxEntity*> found;
        gp_entityManager->findAllEntities(found, 0, test);
        delete test;

        if (!found.empty())
        {
            ++index;
            result = found[index % found.size()]->getId();
        }
    }

    return result;
}

void CEntity::updateSparkTargets(ox::TArray<ox::entity::COxEntity*>& targets)
{
    if (!gp_entityManager || !gp_entityManager->hasBuildingListChanged())
        return;

    targets.clear();
    CSparkListBuilder* test = new CSparkListBuilder(Id, Position);

    int minX = gp_entityManager->calculateGridCoordinateClamp(Position.X - 150.0f);
    int maxX = gp_entityManager->calculateGridCoordinateClamp(Position.X + 150.0f);
    int minY = gp_entityManager->calculateGridCoordinateClamp(Position.Y - 150.0f);
    int maxY = gp_entityManager->calculateGridCoordinateClamp(Position.Y + 150.0f);

    for (int x = minX; x <= maxX; ++x)
    {
        for (int y = minY; y <= maxY; ++y)
        {
            std::list<CEntity*>& cell = gp_entityManager->Grid[0][y * 18 + x];
            for (std::list<CEntity*>::iterator it = cell.begin(); it != cell.end(); ++it)
            {
                if (test->testEntity(*it))
                    targets.push_back(*it);
            }
        }
    }

    delete test;
}

int CEntity::selectSparkTarget(const ox::TArray<ox::entity::COxEntity*>& targets, int excludeId, int& index)
{
    if (targets.empty())
        return 0;

    if (targets.size() == 1)
    {
        if (((CEntity*)targets[0])->wantsSpark())
            return targets[0]->getId();
        return 0;
    }

    int start = index % targets.size();
    int otherId = 0;
    bool foundOther = false;
    bool foundExcluded = false;
    CEntity* entity = (CEntity*)targets[start];
    ++index;

    for (;;)
    {
        if (entity->wantsSpark() && entity->getId() != excludeId && entity->acceptsSparkFrom(getId()))
            return entity->getId();

        if (entity->getId() != excludeId)
        {
            if (!entity->acceptsSparkFrom(getId()))
            {
                otherId = entity->getId();
                foundOther = true;
            }
        }
        else
            foundExcluded = true;

        int current = index % targets.size();
        entity = (CEntity*)targets[current];
        if (current == start)
            break;
        ++index;
    }

    if (foundExcluded)
        return excludeId;
    if (foundOther)
        return otherId;
    return 0;
}

ox::core::CString<wchar_t> CEntity::getInfoString()
{
    return ox::core::CString<wchar_t>(L"");
}

ox::core::CString<wchar_t> CEntity::getMiniStatString()
{
    return ox::core::CString<wchar_t>(L"");
}

ox::core::CString<wchar_t> CEntity::getOperatorString()
{
    return ox::core::CString<wchar_t>(L"");
}

void CEntity::writeEntity(ox::io::IWriteFile* file)
{
    if (Type == 10)
        return;

    ox::io::CHelpIO::writeInt(file, Type);
    ox::io::CHelpIO::writeInt(file, Id);
    ox::io::CHelpIO::writeFloat(file, Position.X);
    ox::io::CHelpIO::writeFloat(file, Position.Y);
    ox::io::CHelpIO::writeInt(file, Killed);
    writeEntityData(file);
}

CEntity* CEntity::readNextEntity(ox::io::IReadFile* file, int version)
{
    int type = ox::io::CHelpIO::readInt(file);
    int id = ox::io::CHelpIO::readInt(file);
    float x = ox::io::CHelpIO::readFloat(file);
    float y = ox::io::CHelpIO::readFloat(file);
    int killed = ox::io::CHelpIO::readInt(file);

    CEntity* entity = 0;

    switch (type)
    {
    case 0:
        entity = new CSparkProducerEntity(x, y);
        break;
    case 1:
        entity = new CSparkMoverEntity(x, y);
        break;
    case 2:
        entity = new CSparkEntity(x, y, -1, -1);
        break;
    case 3:
        entity = new CConstructionEntity(x, y, 0);
        break;
    case 4:
        entity = new CMineralGatherEntity(x, y);
        break;
    case 5:
        entity = new CMineralsEntity(x, y, 0);
        break;
    case 6:
        entity = new CAlienEntity(x, y, 0);
        break;
    case 7:
        entity = new CDefenseTowerEntity(x, y);
        break;
    case 8:
    case 13:
    case 14:
        entity = new CMissileTurretEntity(type, x, y);
        break;
    case 9:
        entity = new CMissileEntity(x, y, ox::core::CPosition2d<float>(0, 0), 0, 0, -1);
        break;
#ifdef HARVEST_PORT
    case 12:
        entity = new CPerimeterBombExplosion(x, y);
        break;
#endif
    case 15:
        entity = new CTempestBlastEntity(ox::core::CVector3d<float>(x, y, 0), ox::core::CVector3d<float>(0, 0, 0),
            -1, -1);
        break;
    case 16:
        entity = new CCreativeEntity(x, y, 0);
        break;
    }

    if (entity)
    {
        entity->Id = id;
        entity->readEntityData(file, version);
        if (killed)
            entity->killEntity();
    }

    return entity;
}

CParticleEntity::CParticleEntity(float x, float y, float z, ox::core::CVector3d<float>* speed,
    const char* particleName)
    : CEntity(0, 10, x, y), ParticleState(0)
{
    Position.Z = z;

    if (gp_particlePackage)
    {
        ParticleState = gp_particlePackage->addNewParticleState(ox::core::CString<char>(particleName));
        if (ParticleState && speed)
            ParticleState->addToSpeed(*speed);
    }
}

CParticleEntity::CParticleEntity(ox::video::IParticleState* state, const ox::core::CVector3d<float>& position)
    : CEntity(0, 10, position.X, position.Y), ParticleState(state)
{
    Position.Z = position.Z;
}

CParticleEntity::~CParticleEntity()
{
    if (ParticleState)
        ParticleState->remove();
}

int CParticleEntity::getRenderLayer() const
{
    if (ParticleState && ParticleState->isGroundSprite())
        return -1;
    return 1;
}

int CParticleEntity::updateLogic(float frameDelta)
{
    if (!ParticleState)
        return true;

    if (!ParticleState->isGroundSprite() && game::gp_world->Planet == 1)
    {
        float windModifier = ParticleState->getWindModifier();
        if (windModifier > 0)
        {
            ox::core::CVector2d<float> wind(0, 0);
            game::gp_world->applyWind(Position, wind, frameDelta);
            wind *= windModifier;
            ParticleState->addToSpeed(ox::core::CVector3d<float>(wind.X, wind.Y, 0));
        }
    }

    if (!ParticleState->update(frameDelta, Position))
        return true;

    if (Position.Z <= 0)
    {
        Position.Z = 0.1f;
        return !ParticleState->notifyBounce(Position, ox::core::CVector3d<float>(0, 0, 1));
    }

    return false;
}

void CParticleEntity::render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort)
{
    if (!ParticleState)
        return;

    ox::core::CPosition2d<float> pos;
    pos.X = Position.X - camera.X + viewPort.UpperLeftCorner.X;
    pos.Y = Position.Y - camera.Y + viewPort.UpperLeftCorner.Y;
    ParticleState->render2DShadow(pos, 1.0f, 0.5f);
    pos.Y -= Position.Z;
    ParticleState->render2D(pos, 1.0f);
}

CSpecialEffectEntity::CSpecialEffectEntity(float x, float y, float z, ox::video::ISpriteAnimationState* sprite,
    float scale, float rotation, ox::video::SColor color)
    : CEntity(0, 20, x, y), Sprite(sprite), Frames(1), Scale(scale), Rotation(rotation), FreeShape(false)
{
    Position.Z = z;
    Color = color;
}

CSpecialEffectEntity::CSpecialEffectEntity(const ox::core::CPosition2d<float>& corner1,
    const ox::core::CPosition2d<float>& corner2, const ox::core::CPosition2d<float>& corner3,
    const ox::core::CPosition2d<float>& corner4, float y, ox::video::ISpriteAnimationState* sprite,
    ox::video::SColor color)
    : CEntity(0, 20, corner1.X, y), Sprite(sprite), Frames(1), Corner1(corner1), Corner2(corner2),
      Corner3(corner3), Corner4(corner4), FreeShape(true)
{
    Corner1.Y *= -1;
    Corner2.Y *= -1;
    Corner3.Y *= -1;
    Corner4.Y *= -1;
    Color = color;
}

CSpecialEffectEntity::~CSpecialEffectEntity()
{
}

int CSpecialEffectEntity::updateLogic(float frameDelta)
{
    if (!Sprite)
        return true;
    return Frames <= 0;
}

void CSpecialEffectEntity::render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort)
{
    if (Position.Z >= 0 && Sprite && !FreeShape)
    {
        ox::core::CPosition2d<float> pos;
        pos.X = Position.X - camera.X + viewPort.UpperLeftCorner.X;
        pos.Y = Position.Y - camera.Y + viewPort.UpperLeftCorner.Y;

        if (Position.Z > 0)
        {
            Sprite->drawRotated(pos, Rotation, Scale, ox::video::SColor(Color.getAlpha() / 2, 0, 0, 0));
            pos.Y -= Position.Z;
        }

        Sprite->drawRotated(pos, Rotation, Scale, Color);
        --Frames;
    }
    else if (Sprite && FreeShape)
    {
        ox::core::CPosition2d<float> offset(viewPort.UpperLeftCorner.X - camera.X,
            viewPort.UpperLeftCorner.Y - camera.Y + Position.Y);
        Sprite->drawFreeShape(Corner1 + offset, Corner2 + offset, Corner3 + offset, Corner4 + offset, Color);
        --Frames;
    }
}

void CSpecialEffectEntity::renderGroundLayer(const ox::core::CPosition2d<float>& camera,
    const ox::core::CRect<int>& viewPort)
{
    if (Position.Z < 0 && Sprite && !FreeShape)
    {
        ox::core::CPosition2d<float> pos;
        pos.X = Position.X - camera.X + viewPort.UpperLeftCorner.X;
        pos.Y = Position.Y - camera.Y + viewPort.UpperLeftCorner.Y;
        Sprite->drawRotated(pos, Rotation, Scale, Color);
        --Frames;
    }
}

} // end namespace entity
} // end namespace harvest
