// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// World mechanics and rendering are reconstructed; byte matching remains partial.

#include <iostream>
#include <math.h>
#include "CWorld.h"
#include "CLuaManager.h"
#include "harvest/entity/CHarvestEntity.h"
#include "harvest/entity/CMineralsEntity.h"
#include "ox/event/IEventReceiver.h"
#include "harvest/gui/CStoryScreen.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/ISpritePackage.h"
#include "ox/video/IParticlePackage.h"
#include "ox/video/IParticleState.h"
#include "ox/core/CMath.h"
#include "ox/core/CBasic.h"
#include "ox/io/CHelpIO.h"

namespace harvest {
namespace game {

//! A moving gust and its five particle emitters.
struct SWindPuff
{
    SWindPuff()
    {
        for (int i = 0; i < 5; ++i) Particles[i] = 0;
    }
    ~SWindPuff()
    {
        for (int i = 0; i < 5; ++i)
            if (Particles[i]) Particles[i]->remove();
    }
    ox::core::CPosition2d<float> Position;
    ox::core::CVector2d<float> Speed;
    float SpeedMagnitude;
    ox::video::IParticleState* Particles[5];
};

CWorld* gp_world = 0;
ox::core::CHiddenInt* gp_mineralAmount = 0;
ox::core::CHiddenInt* gp_negatedMineralAmount = 0;
static const ox::core::CRect<float> STARTING_AREA(450, 390, 630, 570);
static const int DOODAD_TYPES[10] = { 2, 2, 2, 2, 2, 3, 4, 1, 0, 0 };

CWorld::CWorld(int gameMode, int planet)
    : VisibleGameField(0, 0, 1024, 1024), ActualGameField(0, 0, 1024, 1024),
      TargetGameField(0, 0, 1024, 1024), InitialWorld(true), Planet(planet), GameMode(gameMode),
      DoodadGridWidth(0), DoodadGridHeight(0), DoodadGrid(0), WindClock(0)
{
    for (int i = 0; i < 2; ++i) GroundSprites[i] = 0;
    for (int i = 0; i < 23; ++i) DoodadSprites[i] = 0;
    Random.setCurrent(ox::algo::CRand::rand());
}

CWorld::~CWorld()
{
    for (int i = 0; i < 2; ++i) if (GroundSprites[i]) GroundSprites[i]->remove();
    for (int i = 0; i < 23; ++i) if (DoodadSprites[i]) DoodadSprites[i]->remove();
    for (unsigned int i = 0; i < Doodads.size(); ++i) delete Doodads[i];
    delete[] DoodadGrid;
    for (unsigned int i = 0; i < WindPuffs.size(); ++i) delete WindPuffs[i];
}

void CWorld::changeViewSize(const ox::core::CDimension2d<int>& size)
{
    ViewSize.Width = size.Width;
    ViewSize.Height = size.Height;
}

int CWorld::getPlanet() const { return Planet; }
int CWorld::getGameMode() const { return GameMode; }
const ox::core::CRect<float>& CWorld::getActualGameFieldSize() const { return ActualGameField; }
const ox::core::CRect<float>& CWorld::getVisibleGameFieldSize() const { return VisibleGameField; }

bool CWorld::hasWorldExpandedAtLeastOnce()
{
    if (GameMode != 0) return true;
    return TargetGameField.getWidth() > 2048 || TargetGameField.getHeight() > 2048;
}

bool CWorld::worldChangesSizeInThisGameMode() const
{
    if (GameMode == 3 || GameMode == 5) return !InitialWorld;
    return true;
}

void CWorld::constrainViewPos(ox::core::CPosition2d<float>& position)
{
    if (ViewSize.Width > VisibleGameField.getWidth())
        position.X = (VisibleGameField.getWidth() - ViewSize.Width) * .5f;
    else
    {
        if (position.X <= VisibleGameField.UpperLeftCorner.X) position.X = VisibleGameField.UpperLeftCorner.X;
        else if (position.X + ViewSize.Width >= VisibleGameField.LowerRightCorner.X)
            position.X = VisibleGameField.LowerRightCorner.X - ViewSize.Width;
    }
    // The native oversize-height case writes X, rather than Y.
    if (ViewSize.Height > VisibleGameField.getHeight())
        position.X = (VisibleGameField.getHeight() - ViewSize.Height) * .5f;
    else
    {
        if (position.Y <= VisibleGameField.UpperLeftCorner.Y) position.Y = VisibleGameField.UpperLeftCorner.Y;
        else if (position.Y + ViewSize.Height >= VisibleGameField.LowerRightCorner.Y)
            position.Y = VisibleGameField.LowerRightCorner.Y - ViewSize.Height;
    }
}

bool CWorld::checkCollisionWithDoodad(const SDoodad* doodad, const ox::core::CPosition2d<float>& position)
{
    if (!doodad->Bounds.isPointInside(position)) return false;
    float x = position.X - doodad->Position.X;
    float y = (position.Y - doodad->Position.Y) * 1.5f;
    float radius = DoodadCollisionRadii[doodad->Type];
    return x * x + y * y <= radius * radius;
}

bool CWorld::mayMoveHere(const ox::core::CPosition2d<float>& position)
{
    if (DoodadGrid && DoodadGridArea.isPointInside(position))
    {
        int x = (int)((position.X - DoodadGridArea.UpperLeftCorner.X) * .00390625f);
        int y = (int)((position.Y - DoodadGridArea.UpperLeftCorner.Y) * .00390625f);
        int cellIndex = y * DoodadGridWidth + x;
        for (unsigned int i = 0; i < DoodadGrid[cellIndex].size(); ++i)
            if (checkCollisionWithDoodad(DoodadGrid[cellIndex][i], position)) return false;
    }
    return true;
}

bool CWorld::mayPlaceObjectHere(const ox::core::CPosition2d<float>& position, bool building)
{
    if (building && STARTING_AREA.isPointInside(position)) return false;
    if (DoodadGrid && DoodadGridArea.isPointInside(position))
    {
        int x = (int)((position.X - DoodadGridArea.UpperLeftCorner.X) * .00390625f);
        int y = (int)((position.Y - DoodadGridArea.UpperLeftCorner.Y) * .00390625f);
        int cellIndex = y * DoodadGridWidth + x;
        for (unsigned int i = 0; i < DoodadGrid[cellIndex].size(); ++i)
            if (checkCollisionWithDoodad(DoodadGrid[cellIndex][i], position)) return false;
    }
    return true;
}

float CWorld::getCollisionTangent(const ox::core::CPosition2d<float>& position)
{
    if (DoodadGrid && DoodadGridArea.isPointInside(position))
    {
        int x = (int)((position.X - DoodadGridArea.UpperLeftCorner.X) * .00390625f);
        int y = (int)((position.Y - DoodadGridArea.UpperLeftCorner.Y) * .00390625f);
        int cellIndex = y * DoodadGridWidth + x;
        for (unsigned int i = 0; i < DoodadGrid[cellIndex].size(); ++i)
        {
            SDoodad* doodad = DoodadGrid[cellIndex][i];
            if (checkCollisionWithDoodad(doodad, position))
            {
                float dx = position.X - doodad->Position.X;
                float dy = (position.Y - doodad->Position.Y) * 1.5f;
                if (dx == 0) return dy < 0 ? 4.71238899f : 1.57079637f;
                float angle = atanf(dy / dx);
                if (dx < 0) angle += 3.14159274f;
                return angle;
            }
        }
    }
    return 0;
}

ox::core::CPosition2d<float> CWorld::findRendezvousPoint(
    const ox::core::CPosition2d<float>& position, const ox::core::CVector2d<float>& movement)
{
    if (!DoodadGrid || !DoodadGridArea.isPointInside(position))
        return ox::core::CPosition2d<float>(0, 0);
    int x = (int)((position.X - DoodadGridArea.UpperLeftCorner.X) * .00390625f);
    int y = (int)((position.Y - DoodadGridArea.UpperLeftCorner.Y) * .00390625f);
    int cellIndex = y * DoodadGridWidth + x;
    for (unsigned int i = 0; i < DoodadGrid[cellIndex].size(); ++i)
    {
        if (!checkCollisionWithDoodad(DoodadGrid[cellIndex][i], position)) continue;
        float angle = ox::core::CMath::getAngleIY(DoodadGrid[cellIndex][i]->Position, position);
        if (angle <= 0 && angle > -.785398185f)
            angle = ox::core::abs_(movement.X) > ox::core::abs_(movement.Y) ? 4.71238899f : 0;
        else if (angle <= 1.57079637f && angle > 0)
            angle = ox::core::abs_(movement.X) > ox::core::abs_(movement.Y) ? 1.57079637f : 0;
        else if (angle <= 3.14159274f && angle > 1.57079637f)
            angle = ox::core::abs_(movement.X) > ox::core::abs_(movement.Y) ? 1.57079637f : 3.14159274f;
        else if (angle <= 4.71238899f && angle > 3.14159274f)
            angle = ox::core::abs_(movement.X) > ox::core::abs_(movement.Y) ? 4.71238899f : 3.14159274f;
        else
            angle = ox::core::abs_(movement.X) > ox::core::abs_(movement.Y) ? 4.71238899f : 0;
        float radius = DoodadCollisionRadii[DoodadGrid[cellIndex][i]->Type];
        return ox::core::CPosition2d<float>(DoodadGrid[cellIndex][i]->Position.X + cos((double)angle) * (radius + 40),
            DoodadGrid[cellIndex][i]->Position.Y + sin((double)angle) * (radius * .691999972f + 40));
    }
    return ox::core::CPosition2d<float>(position);
}

void CWorld::applyWind(const ox::core::CVector3d<float>& position,
    ox::core::CVector2d<float>& speed, float frameDelta) const
{
    if (Planet != 1) return;
    for (unsigned int i = 0; i < WindPuffs.size(); ++i)
    {
        float distance = ox::core::CMath::getEstimateDistance(
            ox::core::CPosition2d<float>(WindPuffs[i]->Position.X, WindPuffs[i]->Position.Y),
            ox::core::CPosition2d<float>(position.X, position.Y));
        if (distance < 200)
        {
            float amount = (1.0f - distance / 200.0f) * frameDelta;
            speed.X += WindPuffs[i]->Speed.X * amount;
            speed.Y += WindPuffs[i]->Speed.Y * amount;
        }
    }
    speed.X -= 5.0f * frameDelta;
}

void CWorld::createDoodad(const ox::core::CPosition2d<float>& position, int type)
{
    SDoodad* doodad = new SDoodad;
    doodad->Type = type;
    doodad->Position = position;
    float halfHeight = DoodadSizes[type].Height >> 1;
    float halfWidth = DoodadSizes[type].Width >> 1;
    doodad->Bounds = ox::core::CRect<float>(position.X - halfWidth, position.Y - halfHeight,
        position.X + halfWidth, position.Y + halfHeight);
    Doodads.push_back(doodad);
}

void CWorld::placeDoodads(const ox::core::CRect<float>& area, int count)
{
    for (int i = 0; i < count; ++i)
    {
        int type = 0;
        if (Planet == 2)
        {
            int choice = Random.nextInt(10);
            if ((unsigned int)choice <= 9) type = DOODAD_TYPES[choice];
            type = i == 0 ? 0 : type;
        }
        else if (Planet == 0) type = Random.nextInt(6) + 5;
        else if (Planet == 1) type = Random.nextInt(12) + 11;
        ox::core::CPosition2d<float> position;
        bool valid = false;
        for (int attempt = 0; attempt < 4 && !valid; ++attempt)
        {
            position.X = area.UpperLeftCorner.X + Random.nextInt((int)area.getWidth());
            position.Y = area.UpperLeftCorner.Y + Random.nextInt((int)area.getHeight());
            float halfHeight = DoodadSizes[type].Height >> 1;
            float halfWidth = DoodadSizes[type].Width >> 1;
            ox::core::CRect<float> bounds(position.X - halfWidth, position.Y - halfHeight,
                position.X + halfWidth, position.Y + halfHeight);
            if (bounds.isRectCollided(STARTING_AREA)) continue;
            valid = true;
            if (Planet == 2)
                for (unsigned int j = 0; j < Doodads.size(); ++j)
                    if (Doodads[j]->Bounds.isRectCollided(bounds))
                    {
                        valid = false;
                        break;
                    }
        }
        if (valid) createDoodad(position, type);
    }
    recreateDoodadGrid();
}

void CWorld::recreateDoodadGrid()
{
    delete[] DoodadGrid;
    DoodadGrid = 0;
    if (Planet != 2) return;
    DoodadGridArea = TargetGameField;
    DoodadGridArea.UpperLeftCorner -= ox::core::CPosition2d<float>(512, 512);
    DoodadGridArea.LowerRightCorner += ox::core::CPosition2d<float>(512, 512);
    DoodadGridWidth = (int)(DoodadGridArea.getWidth() * .00390625f);
    DoodadGridHeight = (int)(DoodadGridArea.getHeight() * .00390625f);
    DoodadGrid = new ox::TArray<SDoodad*>[DoodadGridWidth * DoodadGridHeight];
    for (int y = 0; y < DoodadGridHeight; ++y)
    {
        for (int x = 0; x < DoodadGridWidth; ++x)
        {
            ox::core::CRect<float> cellArea;
            cellArea.UpperLeftCorner.Y = y * 256.0f + DoodadGridArea.UpperLeftCorner.Y;
            cellArea.UpperLeftCorner.X = x * 256.0f + DoodadGridArea.UpperLeftCorner.X;
            cellArea.LowerRightCorner.X = cellArea.UpperLeftCorner.X + 256;
            cellArea.LowerRightCorner.Y = cellArea.UpperLeftCorner.Y + 256;
            for (unsigned int i = 0; i < Doodads.size(); ++i)
            {
                const ox::core::CRect<float>& bounds = Doodads[i]->Bounds;
                if (bounds.isRectCollided(cellArea))
                    DoodadGrid[y * DoodadGridWidth + x].push_back(Doodads[i]);
            }
        }
    }
}

bool CWorld::write(ox::io::IWriteFile* file)
{
    ox::io::CHelpIO::writeFloat(file, TargetGameField.UpperLeftCorner.X);
    ox::io::CHelpIO::writeFloat(file, TargetGameField.UpperLeftCorner.Y);
    ox::io::CHelpIO::writeFloat(file, TargetGameField.LowerRightCorner.X);
    ox::io::CHelpIO::writeFloat(file, TargetGameField.LowerRightCorner.Y);
    ox::io::CHelpIO::writeFloat(file, VisibleGameField.UpperLeftCorner.X);
    ox::io::CHelpIO::writeFloat(file, VisibleGameField.UpperLeftCorner.Y);
    ox::io::CHelpIO::writeFloat(file, VisibleGameField.LowerRightCorner.X);
    ox::io::CHelpIO::writeFloat(file, VisibleGameField.LowerRightCorner.Y);
    ox::io::CHelpIO::writeFloat(file, ActualGameField.UpperLeftCorner.X);
    ox::io::CHelpIO::writeFloat(file, ActualGameField.UpperLeftCorner.Y);
    ox::io::CHelpIO::writeFloat(file, ActualGameField.LowerRightCorner.X);
    ox::io::CHelpIO::writeFloat(file, ActualGameField.LowerRightCorner.Y);
    ox::io::CHelpIO::writeInt(file, Planet);
    ox::io::CHelpIO::writeInt(file, Doodads.size());
    for (unsigned int i = 0; i < (unsigned int)Doodads.size(); ++i)
    {
        ox::io::CHelpIO::writeInt(file, Doodads[i]->Type);
        ox::io::CHelpIO::writeFloat(file, Doodads[i]->Position.X);
        ox::io::CHelpIO::writeFloat(file, Doodads[i]->Position.Y);
    }
    return true;
}

ox::core::CRect<float> CWorld::expandWorld(int direction, float boundary, bool populate)
{
    ox::core::CRect<float> sceneryArea;
    ox::core::CRect<float> addedArea;
    switch (direction)
    {
    case 0:
        sceneryArea = ox::core::CRect<float>(TargetGameField.UpperLeftCorner.X, boundary - 512,
            TargetGameField.LowerRightCorner.X, TargetGameField.UpperLeftCorner.Y - 512);
        addedArea = ox::core::CRect<float>(TargetGameField.UpperLeftCorner.X, boundary,
            TargetGameField.LowerRightCorner.X, TargetGameField.UpperLeftCorner.Y);
        TargetGameField.UpperLeftCorner.Y = boundary;
        break;
    case 1:
        sceneryArea = ox::core::CRect<float>(boundary - 512, TargetGameField.UpperLeftCorner.Y,
            TargetGameField.UpperLeftCorner.X - 512, TargetGameField.LowerRightCorner.Y);
        addedArea = ox::core::CRect<float>(boundary, TargetGameField.UpperLeftCorner.Y,
            TargetGameField.UpperLeftCorner.X, TargetGameField.LowerRightCorner.Y);
        TargetGameField.UpperLeftCorner.X = boundary;
        break;
    case 2:
        sceneryArea = ox::core::CRect<float>(TargetGameField.UpperLeftCorner.X, TargetGameField.LowerRightCorner.Y + 512,
            TargetGameField.LowerRightCorner.X, boundary + 512);
        addedArea = ox::core::CRect<float>(TargetGameField.UpperLeftCorner.X, TargetGameField.LowerRightCorner.Y,
            TargetGameField.LowerRightCorner.X, boundary);
        TargetGameField.LowerRightCorner.Y = boundary;
        break;
    case 3:
        sceneryArea = ox::core::CRect<float>(TargetGameField.LowerRightCorner.X + 512, TargetGameField.UpperLeftCorner.Y,
            boundary + 512, TargetGameField.LowerRightCorner.Y);
        addedArea = ox::core::CRect<float>(TargetGameField.LowerRightCorner.X, TargetGameField.UpperLeftCorner.Y,
            boundary, TargetGameField.LowerRightCorner.Y);
        TargetGameField.LowerRightCorner.X = boundary;
        break;
    }
    placeDoodads(sceneryArea, ((int)sceneryArea.getWidth() / 512) * ((int)sceneryArea.getHeight() / 512) * 8);
    if (populate)
        for (float y = addedArea.UpperLeftCorner.Y; y < addedArea.LowerRightCorner.Y; y += 512)
            for (float x = addedArea.UpperLeftCorner.X; x < addedArea.LowerRightCorner.X; x += 512)
            {
                ox::core::CRect<float> cell(x, y, x + 512, y + 512);
                float distance = ox::core::CMath::getEstimateDistance(
                    ox::core::CPosition2d<float>((cell.UpperLeftCorner.X + cell.LowerRightCorner.X) * .5f,
                        (cell.UpperLeftCorner.Y + cell.LowerRightCorner.Y) * .5f),
                    ox::core::CPosition2d<float>(512, 512)) * .001953125f;
                int minerals = (int)(30 - (distance * 4 + distance * .5f * distance));
                // The native bonus applies to the inclusive range -24..-16.
                if ((unsigned int)(minerals + 24) < 9 || minerals > 20) minerals = 20;
                if (minerals < 2) minerals = 2;
                entity::CMineralsEntity::fillAreaWithMinerals(cell, minerals, this);
            }
    if (GameMode == 0 && (TargetGameField.getWidth() > 2048 || TargetGameField.getHeight() > 2048))
    {
        ox::event::SEvent event;
        event.EventType = ox::event::EET_USER_EVENT;
        event.UserEvent.UserData1 = 22;
        event.UserEvent.UserData2 = 27;
        event.UserEvent.UserData3 = 0;
        event.UserEvent.UserPointer = 0;
        ox::event::gp_subscriberList->OnEvent(event);
    }
    if (TargetGameField.getWidth() * TargetGameField.getHeight() >= 10485760)
    {
        ox::event::SEvent event;
        event.EventType = ox::event::EET_USER_EVENT;
        event.UserEvent.UserData1 = 21;
        event.UserEvent.UserData2 = 15;
        event.UserEvent.UserData3 = 0;
        event.UserEvent.UserPointer = 0;
        ox::event::gp_subscriberList->OnEvent(event);
        ox::event::SEvent secondEvent;
        secondEvent.EventType = ox::event::EET_USER_EVENT;
        secondEvent.UserEvent.UserData1 = 21;
        secondEvent.UserEvent.UserData2 = 16;
        secondEvent.UserEvent.UserData3 = 0;
        secondEvent.UserEvent.UserPointer = 0;
        ox::event::gp_subscriberList->OnEvent(secondEvent);
    }
    if (gp_luaManager) gp_luaManager->hookMapExpanded(addedArea.UpperLeftCorner.X, addedArea.UpperLeftCorner.Y,
        addedArea.LowerRightCorner.X, addedArea.LowerRightCorner.Y);
    return ox::core::CRect<float>(addedArea);
}

void CWorld::expandWorldFromCurrent(int direction, bool populate)
{
    switch (direction)
    {
    case 0: expandWorld(0, ActualGameField.UpperLeftCorner.Y - 512, populate); break;
    case 1: expandWorld(1, ActualGameField.UpperLeftCorner.X - 512, populate); break;
    case 2: expandWorld(2, ActualGameField.LowerRightCorner.Y + 512, populate); break;
    case 3: expandWorld(3, ActualGameField.LowerRightCorner.X + 512, populate); break;
    }
    ActualGameField = TargetGameField;
    VisibleGameField = TargetGameField;
}

bool CWorld::initializeWorld(ox::video::IVideoDriver* driver, const ox::core::CDimension2d<int>& size)
{
    if (!driver) return false;
    VideoDriver = driver;
    ox::video::ISpritePackage* package = driver->getSpritePackage("$GAME_RESOURCES$/harvestClientData/gfx/ingame.dat", false);
    if (!package) return false;
    if (Planet == 2) GroundSprites[0] = package->addNewAnimationState("Background2");
    else if (Planet == 1) GroundSprites[0] = package->addNewAnimationState("Background3");
    else GroundSprites[0] = package->addNewAnimationState("Background");
    if (!GroundSprites[0]) return false;
    GroundSprites[1] = package->addNewAnimationState("MapShadeEdge");
    if (Planet == 2)
    {
        DoodadSprites[0] = package->addNewAnimationState("Hole1");
        DoodadSprites[1] = package->addNewAnimationState("Hole2");
        DoodadSprites[2] = package->addNewAnimationState("Hole3");
        DoodadSprites[3] = package->addNewAnimationState("Hole5");
        DoodadSprites[4] = package->addNewAnimationState("Hole6");
    }
    else if (Planet == 0)
    {
        DoodadSprites[5] = package->addNewAnimationState("B1Detail1");
        DoodadSprites[6] = package->addNewAnimationState("B1Detail2");
        DoodadSprites[7] = package->addNewAnimationState("B1Detail3");
        DoodadSprites[8] = package->addNewAnimationState("B1Detail4");
        DoodadSprites[9] = package->addNewAnimationState("B1Detail5");
        DoodadSprites[10] = package->addNewAnimationState("B1Detail6");
    }
    else if (Planet == 1)
    {
        DoodadSprites[11] = package->addNewAnimationState("B3Detail1");
        DoodadSprites[12] = package->addNewAnimationState("B3Detail2");
        DoodadSprites[13] = package->addNewAnimationState("B3Detail3");
        DoodadSprites[14] = package->addNewAnimationState("B3Detail4");
        DoodadSprites[15] = package->addNewAnimationState("B3Detail5");
        DoodadSprites[16] = package->addNewAnimationState("B3Detail6");
        DoodadSprites[17] = package->addNewAnimationState("B3Detail7");
        DoodadSprites[18] = package->addNewAnimationState("B3Detail8");
        DoodadSprites[19] = package->addNewAnimationState("B3Detail9");
        DoodadSprites[20] = package->addNewAnimationState("B3Detail10");
        DoodadSprites[21] = package->addNewAnimationState("B3Detail11");
        DoodadSprites[22] = package->addNewAnimationState("B3Detail12");
    }
    for (int i = 0; i < 23; ++i)
        if (DoodadSprites[i])
        {
            ox::core::CDimension2d<int> size = DoodadSprites[i]->getFrameSize(0);
            DoodadSizes[i].Width = size.Width;
            DoodadSizes[i].Height = size.Height;
        }
    DoodadCollisionRadii[0] = 214;
    DoodadCollisionRadii[1] = 146;
    DoodadCollisionRadii[2] = 109;
    DoodadCollisionRadii[3] = 107;
    DoodadCollisionRadii[4] = 89;
    changeViewSize(size);
    return true;
}

void CWorld::initializeNewGame(IScenario* scenario)
{
    if (scenario) Random.setCurrent(scenario->getDoodadSeed());
    ox::core::CRect<float> sceneryArea = TargetGameField;
    sceneryArea.UpperLeftCorner -= ox::core::CPosition2d<float>(512, 512);
    sceneryArea.LowerRightCorner += ox::core::CPosition2d<float>(512, 512);
    placeDoodads(sceneryArea, 128);
    if (scenario) scenario->applyInitialExpansions(this);
    else entity::CMineralsEntity::fillAreaWithMinerals(ActualGameField, 100, this);
    if (GameMode == 3)
        for (int i = 0; i < 4; ++i)
        {
            expandWorld(1, ActualGameField.UpperLeftCorner.X - 512, true);
            expandWorld(3, ActualGameField.LowerRightCorner.X + 512, true);
            expandWorld(0, ActualGameField.UpperLeftCorner.Y - 512, true);
            expandWorld(2, ActualGameField.LowerRightCorner.Y + 512, true);
            ActualGameField = TargetGameField;
        }
}

bool CWorld::readAndInitialize(ox::io::IReadFile* file, int version, ox::video::IVideoDriver* driver,
    const ox::core::CDimension2d<int>& size)
{
    TargetGameField.UpperLeftCorner.X = ox::io::CHelpIO::readFloat(file);
    TargetGameField.UpperLeftCorner.Y = ox::io::CHelpIO::readFloat(file);
    TargetGameField.LowerRightCorner.X = ox::io::CHelpIO::readFloat(file);
    TargetGameField.LowerRightCorner.Y = ox::io::CHelpIO::readFloat(file);
    if (version >= 14)
    {
        VisibleGameField.UpperLeftCorner.X = ox::io::CHelpIO::readFloat(file);
        VisibleGameField.UpperLeftCorner.Y = ox::io::CHelpIO::readFloat(file);
        VisibleGameField.LowerRightCorner.X = ox::io::CHelpIO::readFloat(file);
        VisibleGameField.LowerRightCorner.Y = ox::io::CHelpIO::readFloat(file);
        ActualGameField.UpperLeftCorner.X = ox::io::CHelpIO::readFloat(file);
        ActualGameField.UpperLeftCorner.Y = ox::io::CHelpIO::readFloat(file);
        ActualGameField.LowerRightCorner.X = ox::io::CHelpIO::readFloat(file);
        ActualGameField.LowerRightCorner.Y = ox::io::CHelpIO::readFloat(file);
    }
    else
    {
        VisibleGameField = TargetGameField;
        ActualGameField = TargetGameField;
    }
    if (version >= 6)
    {
        Planet = ox::io::CHelpIO::readInt(file);
        initializeWorld(driver, size);
        int count = ox::io::CHelpIO::readInt(file);
        for (int i = 0; i < count; ++i)
        {
            ox::core::CPosition2d<float> position;
            int type = ox::io::CHelpIO::readInt(file);
            position.X = ox::io::CHelpIO::readFloat(file);
            position.Y = ox::io::CHelpIO::readFloat(file);
            createDoodad(position, type);
        }
    }
    else
    {
        Planet = 0;
        initializeWorld(driver, size);
    }
    recreateDoodadGrid();
    return true;
}

void CWorld::renderBackground(const ox::core::CPosition2d<float>& position, ox::gui::IGUIFont* font,
    ox::core::CRect<int>* clip)
{
    if (!GroundSprites[0]) return;
    ox::core::CDimension2d<float> size = ViewSize;
    int offsetX = 0, offsetY = 0;
    if (clip)
    {
        offsetY = clip->UpperLeftCorner.Y;
        size.Height = clip->getHeight();
        offsetX = clip->UpperLeftCorner.X;
        size.Width = clip->getWidth();
    }
    int tileX = (int)(position.X - 256);
    if (tileX < 0) tileX = ~(-tileX >> 9);
    else tileX >>= 9;
    int tileY = (int)(position.Y - 256);
    if (tileY < 0) tileY = ~(-tileY >> 9);
    else tileY >>= 9;
    // The native renderer computes the tile origin once before drawing the grid.
    ox::core::CPosition2d<int> start((int)(tileX * 512.0f - position.X),
        (int)(tileY * 512.0f - position.Y));
    for (int y = start.Y; y < size.Height; y += 512)
        for (int x = start.X; x < size.Width; x += 512)
            GroundSprites[0]->draw(ox::core::CPosition2d<int>(offsetX + x, offsetY + y), clip, 0xffffffff);
    for (unsigned int i = 0; i < Doodads.size(); ++i)
    {
        SDoodad* doodad = Doodads[i];
        if (DoodadSprites[doodad->Type])
            DoodadSprites[doodad->Type]->draw(ox::core::CPosition2d<int>(offsetX + (int)(doodad->Position.X - position.X),
                offsetY + (int)(doodad->Position.Y - position.Y)), clip, 0xffffffff);
    }
}

void CWorld::renderEdgeShades(const ox::core::CPosition2d<float>& position)
{
    if (!GroundSprites[1]) return;
    ox::core::CPosition2d<float> upper = VisibleGameField.UpperLeftCorner + ox::core::CPosition2d<float>(-6, -6);
    ox::core::CPosition2d<float> lower = VisibleGameField.LowerRightCorner + ox::core::CPosition2d<float>(6, 6);
    ox::core::CRect<float> edges(upper, lower);
    ox::core::CPosition2d<float> point;
    point.Y = edges.UpperLeftCorner.Y - position.Y;
    if (point.Y > -512)
        for (float x = edges.UpperLeftCorner.X; x < edges.LowerRightCorner.X; x += 128)
        {
            point.X = x - position.X;
            GroundSprites[1]->drawRotated(point, 0, 1, 0x80000000);
        }
    point.X = edges.LowerRightCorner.X - position.X;
    if (point.X < ViewSize.Width + 512)
        for (float y = edges.UpperLeftCorner.Y; y < edges.LowerRightCorner.Y; y += 128)
        {
            point.Y = y - position.Y;
            GroundSprites[1]->drawRotated(point, 1.57079637f, 1, 0x80000000);
        }
    point.Y = edges.LowerRightCorner.Y - position.Y;
    if (point.Y < ViewSize.Height + 512)
        for (float x = edges.UpperLeftCorner.X; x < edges.LowerRightCorner.X; x += 128)
        {
            point.X = x - position.X;
            GroundSprites[1]->drawRotated(point + ox::core::CPosition2d<float>(128, 0), 3.14159274f, 1, 0x80000000);
        }
    point.X = edges.UpperLeftCorner.X - position.X;
    if (point.X > -512)
        for (float y = edges.UpperLeftCorner.Y; y < edges.LowerRightCorner.Y; y += 128)
        {
            point.Y = y - position.Y;
            GroundSprites[1]->drawRotated(point + ox::core::CPosition2d<float>(0, 128), 4.71238899f, 1, 0x80000000);
        }
}

void CWorld::update(float frameDelta, const ox::core::CRect<float>& area)
{
    if (Planet == 1)
    {
        WindArea = ox::core::CRect<float>(VisibleGameField.UpperLeftCorner.X - 200,
            VisibleGameField.UpperLeftCorner.Y - 200, VisibleGameField.LowerRightCorner.X + 200,
            VisibleGameField.LowerRightCorner.Y + 200);
        ox::TArray<SWindPuff*>::iterator it = WindPuffs.begin();
        while (it != WindPuffs.end())
        {
            SWindPuff* puff = *it;
            puff->Position += ox::core::CPosition2d<float>(puff->Speed.X * frameDelta, puff->Speed.Y * frameDelta);
            if (!WindArea.isPointInside(puff->Position))
            {
                delete puff;
                it = WindPuffs.erase(it);
            }
            else
            {
                float angle = (float)puff->Speed.getAngle() * .0174532905f;
                float amount = puff->SpeedMagnitude * frameDelta * .00666666683f;
                float ANGLE_OFFS[5] = { 0, .3f, -.3f, .75f, -.75f };
                float SPEED_SCALES[5] = { 1.6f, 1.1f, 1.1f, .2f, .2f };
                for (int i = 0; i < 5; ++i)
                    if (puff->Particles[i])
                    {
                        float cs = cosf(angle + ANGLE_OFFS[i]);
                        float sn = sinf(angle + ANGLE_OFFS[i]);
                        ox::core::CVector3d<float> position(puff->Position.X + cs * 180, puff->Position.Y - sn * 180, 0);
                        puff->Particles[i]->setCurrentSpeed(ox::core::CVector3d<float>(cs * amount, sn * -amount, 0));
                        puff->Particles[i]->update(amount * SPEED_SCALES[i], position);
                    }
                ++it;
            }
        }
        WindClock -= frameDelta;
        if (WindClock <= 0)
        {
            WindClock += (ox::algo::CRand::rand() % 5000) * .001f /
                (WindArea.getHeight() * .001953125f * .5f);
            SWindPuff* puff = new SWindPuff;
            puff->Position = ox::core::CPosition2d<float>(WindArea.LowerRightCorner.X - 1,
                WindArea.UpperLeftCorner.Y + ox::algo::CRand::rand() % (int)WindArea.getHeight());
            puff->Speed = ox::core::CVector2d<float>(-(float)(ox::algo::CRand::rand() % 300 + 50),
                (float)(ox::algo::CRand::rand() % 50 - 25));
            puff->SpeedMagnitude = puff->Speed.getLength();
            for (int i = 0; i < 5; ++i)
                puff->Particles[i] = entity::CEntity::gp_particlePackage->addNewParticleState("IceSpawner");
            WindPuffs.push_back(puff);
        }
    }
    if (!((GameMode == 3 || GameMode == 5) && InitialWorld))
    {
        ActualGameField.UpperLeftCorner.X = ox::core::min_(0.0f, (float)((int)(area.UpperLeftCorner.X - 512) >> 9) * 512);
        ActualGameField.UpperLeftCorner.Y = ox::core::min_(0.0f, (float)((int)(area.UpperLeftCorner.Y - 512) >> 9) * 512);
        ActualGameField.LowerRightCorner.X = ox::core::max_(1024.0f, (float)((int)(area.LowerRightCorner.X + 1024) >> 9) * 512);
        ActualGameField.LowerRightCorner.Y = ox::core::max_(1024.0f, (float)((int)(area.LowerRightCorner.Y + 1024) >> 9) * 512);
        if (GameMode == 4 && gp_luaManager)
        {
            const ox::core::CRect<float>& borders = gp_luaManager->getMinimumWorldBorders();
            ActualGameField.UpperLeftCorner.X = ox::core::min_(ActualGameField.UpperLeftCorner.X, borders.UpperLeftCorner.X);
            ActualGameField.UpperLeftCorner.Y = ox::core::min_(ActualGameField.UpperLeftCorner.Y, borders.UpperLeftCorner.Y);
            ActualGameField.LowerRightCorner.X = ox::core::max_(ActualGameField.LowerRightCorner.X, borders.LowerRightCorner.X);
            ActualGameField.LowerRightCorner.Y = ox::core::max_(ActualGameField.LowerRightCorner.Y, borders.LowerRightCorner.Y);
        }
        ActualGameField.UpperLeftCorner.X = ox::core::max_(ActualGameField.UpperLeftCorner.X, -4096.0f);
        ActualGameField.UpperLeftCorner.Y = ox::core::max_(ActualGameField.UpperLeftCorner.Y, -4096.0f);
        ActualGameField.LowerRightCorner.X = ox::core::min_(ActualGameField.LowerRightCorner.X, 5120.0f);
        ActualGameField.LowerRightCorner.Y = ox::core::min_(ActualGameField.LowerRightCorner.Y, 5120.0f);
    }
    else ActualGameField = TargetGameField;
    if (ActualGameField.UpperLeftCorner.X < TargetGameField.UpperLeftCorner.X)
        expandWorld(1, ActualGameField.UpperLeftCorner.X, true);
    if (ActualGameField.UpperLeftCorner.Y < TargetGameField.UpperLeftCorner.Y)
        expandWorld(0, ActualGameField.UpperLeftCorner.Y, true);
    if (ActualGameField.LowerRightCorner.X > TargetGameField.LowerRightCorner.X)
        expandWorld(3, ActualGameField.LowerRightCorner.X, true);
    if (ActualGameField.LowerRightCorner.Y > TargetGameField.LowerRightCorner.Y)
        expandWorld(2, ActualGameField.LowerRightCorner.Y, true);
    float movement = frameDelta * 100;
    if (ActualGameField.UpperLeftCorner.X > VisibleGameField.UpperLeftCorner.X)
        VisibleGameField.UpperLeftCorner.X = ox::core::min_(VisibleGameField.UpperLeftCorner.X + movement, ActualGameField.UpperLeftCorner.X);
    else if (ActualGameField.UpperLeftCorner.X < VisibleGameField.UpperLeftCorner.X)
        VisibleGameField.UpperLeftCorner.X = ActualGameField.UpperLeftCorner.X;
    if (ActualGameField.UpperLeftCorner.Y > VisibleGameField.UpperLeftCorner.Y)
        VisibleGameField.UpperLeftCorner.Y = ox::core::min_(VisibleGameField.UpperLeftCorner.Y + movement, ActualGameField.UpperLeftCorner.Y);
    else if (ActualGameField.UpperLeftCorner.Y < VisibleGameField.UpperLeftCorner.Y)
        VisibleGameField.UpperLeftCorner.Y = ActualGameField.UpperLeftCorner.Y;
    if (ActualGameField.LowerRightCorner.X < VisibleGameField.LowerRightCorner.X)
        VisibleGameField.LowerRightCorner.X = ox::core::max_(VisibleGameField.LowerRightCorner.X - movement, ActualGameField.LowerRightCorner.X);
    else if (ActualGameField.LowerRightCorner.X > VisibleGameField.LowerRightCorner.X)
        VisibleGameField.LowerRightCorner.X = ActualGameField.LowerRightCorner.X;
    if (ActualGameField.LowerRightCorner.Y < VisibleGameField.LowerRightCorner.Y)
        VisibleGameField.LowerRightCorner.Y = ox::core::max_(VisibleGameField.LowerRightCorner.Y - movement, ActualGameField.LowerRightCorner.Y);
    else if (ActualGameField.LowerRightCorner.Y > VisibleGameField.LowerRightCorner.Y)
        VisibleGameField.LowerRightCorner.Y = ActualGameField.LowerRightCorner.Y;
}

} // end namespace game
} // end namespace harvest
