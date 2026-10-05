// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include <algorithm>
#include <vector>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CShuttleEntity.h"
#include "harvest/entity/CEntityManager.h"
#include "ox/algo/CRand.h"
#include "ox/core/CBasic.h"
#include "ox/core/CMath.h"
#include "ox/audio/IAudioDriver.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IParticlePackage.h"
#include "ox/video/IParticleState.h"
namespace harvest {
namespace entity {

bool g_checkPointsInited = false;
ox::core::CPosition2d<float> g_bestEndTargets[14] = {
    ox::core::CPosition2d<float>(500.0f, 1580.0f),
    ox::core::CPosition2d<float>(878.400024f, 1570.94995f),
    ox::core::CPosition2d<float>(1236.69995f, 1036.35999f),
    ox::core::CPosition2d<float>(1125.0f, 730.0f),
    ox::core::CPosition2d<float>(884.26001f, 391.48999f),
    ox::core::CPosition2d<float>(410.970001f, 288.850006f),
    ox::core::CPosition2d<float>(141.199997f, -84.5199966f),
    ox::core::CPosition2d<float>(22.2900009f, -502.649994f),
    ox::core::CPosition2d<float>(-587.169983f, -513.659973f),
    ox::core::CPosition2d<float>(-627.52002f, -164.0f),
    ox::core::CPosition2d<float>(-665.330017f, 294.0f),
    ox::core::CPosition2d<float>(-754.169983f, 837.340027f),
    ox::core::CPosition2d<float>(-544.210022f, 1327.45996f),
    ox::core::CPosition2d<float>(50.0f, 1563.0f),
};
ox::core::CPosition2d<float> g_bestMiddleTargets[14] = {
    ox::core::CPosition2d<float>(275.0f, 1571.5f),
    ox::core::CPosition2d<float>(745.919983f, 1619.60999f),
    ox::core::CPosition2d<float>(1123.80005f, 1239.18005f),
    ox::core::CPosition2d<float>(1171.93005f, 858.23999f),
    ox::core::CPosition2d<float>(1001.02002f, 532.25f),
    ox::core::CPosition2d<float>(649.200012f, 337.600006f),
    ox::core::CPosition2d<float>(205.600006f, 97.2399979f),
    ox::core::CPosition2d<float>(92.2399979f, -344.890015f),
    ox::core::CPosition2d<float>(-413.529999f, -537.690002f),
    ox::core::CPosition2d<float>(-624.960022f, -352.0f),
    ox::core::CPosition2d<float>(-643.73999f, 64.6299973f),
    ox::core::CPosition2d<float>(-712.960022f, 447.790009f),
    ox::core::CPosition2d<float>(-675.51001f, 1093.32996f),
    ox::core::CPosition2d<float>(-266.5f, 1461.5f),
};
float g_bestLapTime = 0;
float g_recentBestLapTime = 0;
CShuttleEntity::CShuttleEntity(int ai)
    : CEntity(g_nextEntityId++, 19, LAP_CHECKPOINTS[0].X, LAP_CHECKPOINTS[0].Y), SpriteIndex(0),
      Angle(0), Ai(ai), MovementFlags(0), NextCheckPoint(1), Lap(1), LapTime(0), TotalTime(0),
      LastCheckPointTime(0), Placing(10), ReturnState(0), UseMiddleTarget(true)
{
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
    if (gp_particlePackage)
        Thrust = gp_particlePackage->addNewParticleState(ox::core::CString<char>("TinyAlienThrust"));
    for (int i = 0; i < 14; ++i)
    {
        EndTargets[i] = g_bestEndTargets[i];
        MiddleTargets[i] = g_bestMiddleTargets[i];
        if (Ai != 1)
        {
            EndTargets[i].X += ox::algo::CRand::rand() % 11 - 5;
            EndTargets[i].Y += ox::algo::CRand::rand() % 11 - 5;
            MiddleTargets[i].X += ox::algo::CRand::rand() % 51 - 25;
            MiddleTargets[i].Y += ox::algo::CRand::rand() % 51 - 25;
        }
    }
    updatePlacing();
}

CShuttleEntity::~CShuttleEntity()
{
    for (int i = 0; i < 30; ++i)
        if (Sprites[i]) Sprites[i]->remove();
    if (Thrust) Thrust->remove();
}
void CShuttleEntity::updatePlacing()
{
    std::vector<CShuttleEntity*> shuttles;
    const std::list<ox::entity::COxEntity*>& entities = gp_entityManager->getEntityList(3);
    for (std::list<ox::entity::COxEntity*>::const_iterator it = entities.begin(); it != entities.end(); ++it)
        if ((*it)->getEntityType() == 19)
            shuttles.push_back((CShuttleEntity*)*it);
    std::sort(shuttles.begin(), shuttles.end(), SBestShuttleSorter());
    for (int i = 0; i < (int)shuttles.size(); ++i)
        shuttles[i]->setCurrentPlacing(i + 1);
}
void CShuttleEntity::updateSprite(float frameDelta)
{
    if (Sprites[SpriteIndex]) Sprites[SpriteIndex]->update(frameDelta);
}
int CShuttleEntity::updateLogic(float frameDelta)
{
    LapTime += frameDelta;
    TotalTime += frameDelta;
    if (Ai >= 0) updateAi();
    if (ReturnState == 0)
    {
        if (MovementFlags & 2) Angle += 3.0f * frameDelta;
        if (MovementFlags & 1) Angle += -3.0f * frameDelta;
    }
    while (Angle < 0) Angle += 6.28318548f;
    while (Angle > 6.28318548f) Angle -= 6.28318548f;
    SpriteIndex = 0;
    if (Angle > .104719758f && Angle < 6.17846584f)
        SpriteIndex = 29 - (int)((Angle - .104719758f) / .209439516f);
    if (ReturnState == 0)
    {
        if (MovementFlags & 4)
        {
            Speed.X += cos((double)Angle) * 200.0 * frameDelta;
            Speed.Y += sin((double)Angle) * 200.0 * frameDelta;
        }
        Position.X += Speed.X * frameDelta;
        Position.Y += Speed.Y * frameDelta;
        float friction = ox::core::max_(0.0f, 1.0f - .7f * frameDelta);
        Speed *= friction;
        Color.setAlpha(255);
    }
    else if (ReturnState == 1)
    {
        bool arrivedX = false, arrivedY = false;
        if (Position.X < ReturnPosition.X)
        {
            Position.X += 100.0f * frameDelta;
            if (Position.X >= ReturnPosition.X) { Position.X = ReturnPosition.X; arrivedX = true; }
        }
        else
        {
            Position.X += -100.0f * frameDelta;
            if (Position.X <= ReturnPosition.X) { Position.X = ReturnPosition.X; arrivedX = true; }
        }
        if (Position.Y < ReturnPosition.Y)
        {
            Position.Y += 100.0f * frameDelta;
            if (Position.Y >= ReturnPosition.Y) { Position.Y = ReturnPosition.Y; arrivedY = true; }
        }
        else
        {
            Position.Y += -100.0f * frameDelta;
            if (Position.Y <= ReturnPosition.Y) { Position.Y = ReturnPosition.Y; arrivedY = true; }
        }
        Speed.X = 0; Speed.Y = 0;
        Color.setAlpha(128);
        if (arrivedX && arrivedY) ReturnState = 0;
    }
    if ((MovementFlags & 4) && ReturnState == 0)
    {
        ox::core::CVector3d<float> exhaust(Position.X + cos((double)Angle) * -8.0,
            Position.Y + sin((double)Angle) * -5.0, .1f);
        if (Thrust && !Thrust->update(frameDelta + frameDelta, exhaust))
        {
            Thrust->remove(); Thrust = 0;
        }
    }
    if (ReturnState == 0 && !game::gp_world->mayMoveHere(ox::core::CPosition2d<float>(Position.X, Position.Y)) && LapTime < 1000.0f)
    {
        ox::core::CVector3d<float> speed(Speed.X, Speed.Y, 1.0f);
        gp_entityManager->appendEntity(new CParticleEntity(Position.X, Position.Y, Position.Z + 3.0f,
            &speed, "AlienDeathSmall"), 4);
        int previous = (NextCheckPoint - 1) % 14;
        if (previous < 0) previous += 14;
        ReturnPosition = LAP_CHECKPOINTS[previous];
        ReturnState = 1;
        UseMiddleTarget = true;
        int next = NextCheckPoint % 14;
        MiddleTargets[next].X = (LAP_CHECKPOINTS[next].X + LAP_CHECKPOINTS[previous].X) * .5f;
        MiddleTargets[next].Y = (LAP_CHECKPOINTS[next].Y + LAP_CHECKPOINTS[previous].Y) * .5f;
        EndTargets[next] = LAP_CHECKPOINTS[next];
    }
    float x = Position.X - LAP_CHECKPOINTS[NextCheckPoint % 14].X;
    float y = Position.Y - LAP_CHECKPOINTS[NextCheckPoint % 14].Y;
    if (x * x + y * y <= 15625.0f)
    {
        ++NextCheckPoint;
        LastCheckPointTime = TotalTime;
        if (NextCheckPoint % 14 == 1) { ++Lap; LapTime = 0; }
        int oldPlacing = Placing;
        updatePlacing();
        if (Placing < oldPlacing && Ai < 0)
        {
            switch (ox::algo::CRand::rand() % 3)
            {
            case 0: gp_audioDriver->playSound("Laugh1.ogg", 1, 0, 1); break;
            case 1: gp_audioDriver->playSound("Laugh2.ogg", 1, 0, 1); break;
            case 2: gp_audioDriver->playSound("Laugh3.ogg", 1, 0, 1); break;
            }
        }
        UseMiddleTarget = true;
        if (gp_audioDriver && Ai < 0) gp_audioDriver->playSound("SelectBuilding.ogg", 1, 0, 1);
    }
    return 0;
}
bool CShuttleEntity::isAi() { return Ai >= 0; }
void CShuttleEntity::updateAi()
{
    MovementFlags |= 4;
    ox::core::CPosition2d<float> target = EndTargets[NextCheckPoint % 14];
    if (UseMiddleTarget)
    {
        target = MiddleTargets[NextCheckPoint % 14];
        float x = Position.X - target.X;
        float y = Position.Y - target.Y;
        if (x * x + y * y <= 2500.0f) UseMiddleTarget = false;
        float distance = ox::core::CMath::getEstimateDistance(ox::core::CPosition2d<float>(Position.X, Position.Y),
            LAP_CHECKPOINTS[NextCheckPoint % 14]);
        if (ox::core::CMath::getEstimateDistance(ox::core::CPosition2d<float>(Position.X, Position.Y),
            LAP_CHECKPOINTS[(NextCheckPoint - 1) % 14]) > distance) UseMiddleTarget = false;
    }
    float angle = ox::core::CMath::getAngleIY(ox::core::CPosition2d<float>(Position.X, Position.Y), target);
    if (angle < 0) angle += 6.28318548f;
    else if (angle > 6.28318548f) angle -= 6.28318548f;
    if (ox::core::CMath::clockWiseClosestIY(Angle, angle))
    {
        MovementFlags |= 2;
        MovementFlags &= ~1;
    }
    else
    {
        MovementFlags &= ~2;
        MovementFlags |= 1;
    }
}
int CShuttleEntity::getNextCheckPoint() const { return NextCheckPoint % 14; }
void CShuttleEntity::render(const ox::core::CPosition2d<float>& camera, const ox::core::CRect<int>& viewPort)
{
    float y = Position.Y;
    Position.Y -= Position.Z;
    CEntity::renderSprite(camera, viewPort, Sprites[SpriteIndex]);
    Position.Y = y;
}
void CShuttleEntity::writeEntityData(ox::io::IWriteFile* file) {}
void CShuttleEntity::readEntityData(ox::io::IReadFile* file, int version) {}
void CShuttleEntity::setMovementFlag(int flag, bool enabled)
{
    if (enabled) MovementFlags |= flag;
    else MovementFlags &= ~flag;
}
const ox::core::CVector2d<float>& CShuttleEntity::getCurrentSpeed() { return Speed; }
int CShuttleEntity::getNextCheckPointNoMod() const { return NextCheckPoint; }
int CShuttleEntity::getCurrentLap() const { return Lap; }
float CShuttleEntity::getTimeAtLastCheckPoint() const { return LastCheckPointTime; }
void CShuttleEntity::setCurrentPlacing(int placing) { Placing = placing; }
int CShuttleEntity::getCurrentPlacing() const { return Placing; }
float CShuttleEntity::getCurrentLapTime() const { return LapTime; }
float CShuttleEntity::getTotalTime() const { return TotalTime; }
void CShuttleEntity::resetAllAiByMe(bool copyMine)
{
    NextCheckPoint = 1;
    LapTime = 0;
    for (int i = 0; i < 14; ++i)
    {
        EndTargets[i] = g_bestEndTargets[i];
        MiddleTargets[i] = g_bestMiddleTargets[i];
    }
    const std::list<ox::entity::COxEntity*>& entities = gp_entityManager->getEntityList(3);
    for (std::list<ox::entity::COxEntity*>::const_iterator it = entities.begin(); it != entities.end(); ++it)
    {
        if ((*it)->getEntityType() != 19) continue;
        CShuttleEntity* shuttle = (CShuttleEntity*)*it;
        if (shuttle->Ai < 0 || shuttle->getId() == getId()) continue;
        if (!copyMine)
        {
            // every other AI mutates its own targets
            for (int i = 0; i < 14; ++i)
            {
                shuttle->EndTargets[i].X = ox::core::clamp(shuttle->EndTargets[i].X + (ox::algo::CRand::rand() % 11 - 5),
                    LAP_CHECKPOINTS[i].X - 125.0f, LAP_CHECKPOINTS[i].X + 125.0f);
                shuttle->EndTargets[i].Y = ox::core::clamp(shuttle->EndTargets[i].Y + (ox::algo::CRand::rand() % 11 - 5),
                    LAP_CHECKPOINTS[i].Y - 125.0f, LAP_CHECKPOINTS[i].Y + 125.0f);
                if (ox::algo::CRand::rand() % 150 == 0)
                {
                    shuttle->MiddleTargets[i].X = (LAP_CHECKPOINTS[i].X + LAP_CHECKPOINTS[(i - 1) % 14].X) * .5f;
                    shuttle->MiddleTargets[i].Y = (LAP_CHECKPOINTS[i].Y + LAP_CHECKPOINTS[(i - 1) % 14].Y) * .5f;
                }
                else
                {
                    shuttle->MiddleTargets[i].X += ox::algo::CRand::rand() % 51 - 25;
                    shuttle->MiddleTargets[i].Y += ox::algo::CRand::rand() % 51 - 25;
                }
            }
        }
        else
        {
            // every other AI starts from a jittered copy of this shuttle's targets
            for (int i = 0; i < 14; ++i)
            {
                shuttle->EndTargets[i].X = ox::core::clamp(EndTargets[i].X + (ox::algo::CRand::rand() % 11 - 5),
                    LAP_CHECKPOINTS[i].X - 125.0f, LAP_CHECKPOINTS[i].X + 125.0f);
                shuttle->EndTargets[i].Y = ox::core::clamp(EndTargets[i].Y + (ox::algo::CRand::rand() % 11 - 5),
                    LAP_CHECKPOINTS[i].Y - 125.0f, LAP_CHECKPOINTS[i].Y + 125.0f);
                if (ox::algo::CRand::rand() % 15 == 0)
                {
                    shuttle->MiddleTargets[i].X = (LAP_CHECKPOINTS[i].X + LAP_CHECKPOINTS[(i - 1) % 14].X) * .5f;
                    shuttle->MiddleTargets[i].Y = (LAP_CHECKPOINTS[i].Y + LAP_CHECKPOINTS[(i - 1) % 14].Y) * .5f;
                }
                else
                {
                    shuttle->MiddleTargets[i].X = MiddleTargets[i].X + (ox::algo::CRand::rand() % 51 - 25);
                    shuttle->MiddleTargets[i].Y = MiddleTargets[i].Y + (ox::algo::CRand::rand() % 51 - 25);
                }
            }
        }    }
}
} // end namespace entity
} // end namespace harvest
