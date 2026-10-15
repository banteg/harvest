// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CPerimeterBomb.h"
#include "harvest/entity/CAlienEntity.h"
#include "harvest/entity/CEntityManager.h"
#include "ox/core/CBasic.h"
#include "ox/event/IEventReceiver.h"
#include "ox/io/CHelpIO.h"
#include "ox/video/ISpriteAnimationState.h"
#include "ox/video/ISpritePackage.h"

namespace harvest {
namespace entity {

CPerimeterBombExplosion::CPerimeterBombExplosion(float x, float y)
    : CEntity(g_nextEntityId++, 12, x, y), Fuse(3.0f), Speed(0, 0), Moved(false)
{
    Sprite = gp_spritePackage->addNewAnimationState(ox::core::CString<char>("BombActive"));
}

CPerimeterBombExplosion::~CPerimeterBombExplosion()
{
    if (Sprite)
        Sprite->remove();
}

void CPerimeterBombExplosion::updateSprite(float frameDelta)
{
    if (Sprite)
        Sprite->update(frameDelta);
}

int CPerimeterBombExplosion::updateLogic(float frameDelta)
{
    Position.X += Speed.X * frameDelta;
    Position.Y += Speed.Y * frameDelta;
    if (!Moved)
    {
        if (Speed.getLength() > 10.0)
            Moved = true;
    }
    Speed *= ox::core::max_(0.0f, 1.0f - frameDelta);
    Fuse -= frameDelta;

    if (Fuse <= 0 || (Moved && !game::gp_world->mayMoveHere(
            ox::core::CPosition2d<float>(Position.X, Position.Y))))
    {
        gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y, 3.0f, 0, "BombExplosion"), 4);

        int kills = 0;
        const std::list<ox::entity::COxEntity*>& aliens = gp_entityManager->getEntityList(1);
        for (std::list<ox::entity::COxEntity*>::const_iterator it = aliens.begin(); it != aliens.end(); ++it)
        {
            float dx = Position.X - (*it)->getPosition().X;
            float dy = Position.Y - (*it)->getPosition().Y;
            float distance = dx * dx + dy * dy;
            if (distance <= 40000.0f)
            {
                float damage = (1.0f - distance / 40000.0f) * 130.0f;
                if (((CAlienEntity*)*it)->dealDamage(damage,
                        ox::core::CPosition2d<float>(Position.X, Position.Y), 3.0f, 4))
                    ++kills;
            }
        }

        const std::list<ox::entity::COxEntity*>& bombs = gp_entityManager->getEntityList(3);
        for (std::list<ox::entity::COxEntity*>::const_iterator it = bombs.begin(); it != bombs.end(); ++it)
        {
            if ((*it)->getEntityType() == 12)
            {
                float dx = Position.X - (*it)->getPosition().X;
                float dy = Position.Y - (*it)->getPosition().Y;
                float distance = dx * dx + dy * dy;
                if (distance <= 10000.0f)
                {
                    ox::core::CVector2d<float> direction((*it)->getPosition().X - Position.X,
                        (*it)->getPosition().Y - Position.Y);
                    direction.normalize();
                    ((CPerimeterBombExplosion*)*it)->Speed += direction * ((1.0f - distance / 10000.0f) * 500.0f);
                }
            }
        }

        if (kills >= 20)
        {
            ox::event::SEvent event;
            event.EventType = ox::event::EET_USER_EVENT;
            event.UserEvent.UserData1 = 21;
            event.UserEvent.UserData2 = 20;
            event.UserEvent.UserData3 = 0;
            event.UserEvent.UserPointer = 0;
            ox::event::gp_subscriberList->OnEvent(event);
        }
        if (kills >= 6 && Moved)
        {
            ox::event::SEvent event;
            event.EventType = ox::event::EET_USER_EVENT;
            event.UserEvent.UserData1 = 21;
            event.UserEvent.UserData2 = 11;
            event.UserEvent.UserData3 = 0;
            event.UserEvent.UserPointer = 0;
            ox::event::gp_subscriberList->OnEvent(event);
        }
        return 1;
    }
    return 0;
}

void CPerimeterBombExplosion::render(const ox::core::CPosition2d<float>& camera,
    const ox::core::CRect<int>& viewPort)
{
    CEntity::renderSprite(camera, viewPort, Sprite);
}

void CPerimeterBombExplosion::writeEntityData(ox::io::IWriteFile* file)
{
    ox::io::CHelpIO::writeFloat(file, Speed.X);
    ox::io::CHelpIO::writeFloat(file, Speed.Y);
    ox::io::CHelpIO::writeFloat(file, Fuse);
}

void CPerimeterBombExplosion::readEntityData(ox::io::IReadFile* file, int version)
{
    if (version >= 6)
    {
        Speed.X = ox::io::CHelpIO::readFloat(file);
        Speed.Y = ox::io::CHelpIO::readFloat(file);
        Fuse = ox::io::CHelpIO::readFloat(file);
    }
}

} // end namespace entity
} // end namespace harvest
