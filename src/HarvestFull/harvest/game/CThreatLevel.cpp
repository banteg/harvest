// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "harvest/game/CThreatLevel.h"
// The object has an iostream static initializer, set up before the game headers' statics; the
// header that pulled it in is not identified yet.
#include <iostream>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CAlienEntity.h"
#include "harvest/entity/CEntityManager.h"
#include "harvest/settings/CSystemConfig.h"
#include "harvest/ECustomEvents.h"
#include "ox/algo/CArrayFunctions.h"
#include "ox/algo/CRand.h"
#include "ox/core/CBasic.h"
#include "ox/event/IEventReceiver.h"
#include "ox/io/CHelpIO.h"

namespace harvest {
namespace game {

//! A wave of the wave game mode: its name and how many aliens of each type attack from each side.
struct SWaveDefinition
{
    const wchar_t* Name;
    int Aliens[14];
};

static const SWaveDefinition WAVE_DEFINITIONS[3][10] =
{
    {
        { L"Milkys", { 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Milkys", { 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Milkys", { 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Miners", { 0, 0, 0, 0, 0, 50, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Miners", { 0, 0, 0, 0, 0, 50, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Shuttles", { 0, 0, 75, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Shielders", { 0, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Summoners", { 0, 0, 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Thunders", { 0, 0, 0, 0, 0, 0, 0, 0, 20, 0, 0, 0, 0, 0 } },
        { L"Everything", { 20, 10, 10, 5, 0, 15, 0, 0, 5, 0, 0, 0, 0, 0 } }
    },
    {
        { L"Milkys", { 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Milkys", { 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Milkys and Magnetos", { 40, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0 } },
        { L"Lookers", { 0, 0, 0, 0, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Lookers and Magnetos", { 0, 0, 0, 0, 35, 0, 0, 5, 0, 0, 0, 0, 0, 0 } },
        { L"Shielders", { 0, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Shielers and Magnetos", { 0, 20, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0 } },
        { L"Summoners", { 0, 0, 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Summoners and Magnetos", { 0, 0, 0, 15, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0 } },
        { L"Everything", { 20, 10, 0, 5, 15, 0, 0, 5, 0, 0, 0, 0, 0, 0 } }
    },
    {
        { L"Milkys", { 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Milkys", { 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Milkys and Stealers", { 40, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Lookers", { 0, 0, 0, 0, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Lookers and Stealers", { 0, 0, 0, 0, 35, 0, 5, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Shielders", { 0, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Shielders and Stealers", { 0, 20, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0 } },
        { L"Thunders", { 0, 0, 0, 0, 0, 0, 0, 0, 20, 0, 0, 0, 0, 0 } },
        { L"Thunders and Stealers", { 0, 0, 0, 0, 0, 0, 5, 0, 15, 0, 0, 0, 0, 0 } },
        { L"Everything", { 20, 10, 0, 0, 15, 0, 5, 0, 5, 0, 0, 0, 0, 0 } }
    }
};

CThreatLevel::CThreatLevel(int gameMode)
    : Logic(0), GameMode(gameMode)
{
    createLogicForGameMode();
}

void CThreatLevel::createLogicForGameMode()
{
    if (Logic)
        delete Logic;

    switch (GameMode)
    {
    case EGM_NORMAL:
        Logic = new CThreatLevelNormal();
        break;
    case EGM_WAVE:
        Logic = new CThreatLevelWave();
        break;
    case EGM_INSANE:
        Logic = new CThreatLevelInsane();
        break;
    case EGM_RUSH:
        Logic = new CThreatLevelRush();
        break;
    case EGM_CREATIVE:
        Logic = new CThreatLevelCreative();
        break;
    }
}

CThreatLevel::~CThreatLevel()
{
    if (Logic)
        delete Logic;
}

int CThreatLevel::getThreatLevel()
{
    if (!Logic)
        return 0;

    return Logic->ThreatLevel.getValue();
}

float CThreatLevel::getThreatLevelProgress()
{
    float progress = 0.0f;
    if (Logic && Logic->LevelDuration > 0.0f)
        progress = 1.0f - Logic->TimeLeft / Logic->LevelDuration;
    return progress;
}

bool CThreatLevel::read(ox::io::IReadFile* file, int version)
{
    if (version < 16)
        GameMode = EGM_NORMAL;
    else
        GameMode = ox::io::CHelpIO::readInt(file);

    createLogicForGameMode();

    if (Logic)
        return Logic->read(file, version);
    return false;
}

bool CThreatLevelLogic::read(ox::io::IReadFile* file, int version)
{
    AttackTimer = ox::io::CHelpIO::readFloat(file);
    TimeLeft = ox::io::CHelpIO::readFloat(file);
    ThreatLevel.setValue(ox::io::CHelpIO::readInt(file));
    Direction = ox::io::CHelpIO::readInt(file);

    if (version >= 6)
        AttackCount.setValue(ox::io::CHelpIO::readInt(file));

    if (version >= 25)
        SectionSeed.setValue(ox::io::CHelpIO::readInt(file));

    return readGameModeSpecific(file, version);
}

bool CThreatLevel::write(ox::io::IWriteFile* file)
{
    ox::io::CHelpIO::writeInt(file, GameMode);

    if (Logic)
        return Logic->write(file);
    return false;
}

bool CThreatLevelLogic::write(ox::io::IWriteFile* file)
{
    ox::io::CHelpIO::writeFloat(file, AttackTimer);
    ox::io::CHelpIO::writeFloat(file, TimeLeft);
    ox::io::CHelpIO::writeInt(file, ThreatLevel.getValue());
    ox::io::CHelpIO::writeInt(file, Direction);
    ox::io::CHelpIO::writeInt(file, AttackCount.getValue());
    ox::io::CHelpIO::writeInt(file, SectionSeed.getValue());

    return writeGameModeSpecific(file);
}

bool CThreatLevel::update(float frameDelta)
{
    if (Logic)
        return Logic->update(frameDelta);
    return false;
}

bool CThreatLevel::alienOccursOnPlanet(int planet, int alienType)
{
    switch (alienType)
    {
    case 0:
    case 1:
        return true;
    case 2:
    case 3:
        return planet == 0 || planet == 1;
    case 4:
        return planet == 1 || planet == 2;
    case 5:
        return planet == 0;
    case 6:
        return planet == 2;
    case 7:
        return planet == 1;
    case 8:
        return planet == 0 || planet == 2;
    }

    return false;
}

bool CThreatLevel::alienIsPresentAtThisLevel(int alienType)
{
    int planet = gp_world->getPlanet();

    if (!alienOccursOnPlanet(planet, alienType))
        return false;

    if (GameMode == EGM_WAVE || GameMode == EGM_RUSH || GameMode == EGM_CREATIVE)
        return true;

    int level = Logic->ThreatLevel.getValue();

    // the threat level at which each alien type first attacks, per planet
    const int ALIEN_LEVELS[3][14] =
    {
        { 1, 45, 13, 35, 1, 11, 1, 1, 62, 1, 1, 1, 1, 1 },
        { 1, 45, 1, 35, 15, 1, 13, 22, 1, 1, 1, 1, 1, 1 },
        { 1, 23, 1, 1, 22, 1, 63, 1, 13, 1, 1, 1, 1, 1 }
    };

    return level >= ALIEN_LEVELS[planet][alienType];
}

bool CThreatLevel::hasWaveBeenLaunched(int wave)
{
    if (GameMode == EGM_WAVE && Logic)
        return ((CThreatLevelWave*)Logic)->hasWaveBeenLaunched(wave);
    return false;
}

bool CThreatLevel::alienIsPresentOnThisWave(int planet, int wave, int alienType)
{
    if (GameMode == EGM_WAVE && Logic)
        return ((CThreatLevelWave*)Logic)->alienIsPresentOnThisWave(planet, wave, alienType);
    return false;
}

bool CThreatLevel::spawnNextWaveAttack(int wave, ox::core::CString<wchar_t>& message)
{
    if (GameMode != EGM_WAVE || !Logic)
    {
        message = L"Not Wave game mode!";
        return false;
    }

    return ((CThreatLevelWave*)Logic)->spawnNextWaveAttack(wave, message);
}

const wchar_t* CThreatLevel::getWaveDescription(int wave)
{
    if (GameMode == EGM_WAVE && Logic)
        return ((CThreatLevelWave*)Logic)->getWaveDescription(wave);
    return L"";
}

bool CThreatLevel::allowVictory()
{
    if (Logic)
        return Logic->allowVictory();
    return false;
}

int CThreatLevel::getWaveReward()
{
    if (GameMode == EGM_WAVE && Logic)
        return ((CThreatLevelWave*)Logic)->getWaveReward();
    return 0;
}

CThreatLevelLogic::CThreatLevelLogic()
    : TimeLeft(60.0f), LevelDuration(60.0f), AttackTimer(0.0f), ThreatLevel(0), AttackCount(0),
      Direction(0)
{
}

CThreatLevelLogic::~CThreatLevelLogic()
{
}

void CThreatLevelLogic::increaseThreatLevel()
{
    TimeLeft += 60.0f;
    LevelDuration = 60.0f;
    ThreatLevel.modifyValue(1);
    AttackCount.setValue(0);
    AttackTimer = 0.0f;
    SectionSeed.setValue(ox::algo::CRand::rand());
}

void CThreatLevelLogic::initAttackBorderArea(int direction, ox::core::CRect<float>& area)
{
    area = gp_world->getActualGameFieldSize();

    switch (direction)
    {
    case 0:
        area.LowerRightCorner.Y = area.UpperLeftCorner.Y - 50.0f;
        area.UpperLeftCorner.Y = area.UpperLeftCorner.Y - 400.0f;
        break;
    case 1:
        area.LowerRightCorner.X = area.UpperLeftCorner.X - 50.0f;
        area.UpperLeftCorner.X = area.UpperLeftCorner.X - 400.0f;
        break;
    case 2:
        area.UpperLeftCorner.Y = area.LowerRightCorner.Y + 50.0f;
        area.LowerRightCorner.Y = area.LowerRightCorner.Y + 400.0f;
        break;
    case 3:
        area.UpperLeftCorner.X = area.LowerRightCorner.X + 50.0f;
        area.LowerRightCorner.X = area.LowerRightCorner.X + 400.0f;
        break;
    }
}

void CThreatLevelLogic::initAttackSectionArea(int direction, int section, int threatLevel,
    ox::core::CRect<float>& area)
{
    area = gp_world->getActualGameFieldSize();

    float width = threatLevel * 20.0f + 300.0f;
    int sectionsX = (int)(area.getWidth() / 512.0f + 0.5f);
    int sectionsY = (int)(area.getHeight() / 512.0f + 0.5f);

    switch (direction)
    {
    case 0:
        area.LowerRightCorner.Y = area.UpperLeftCorner.Y - 50.0f;
        area.UpperLeftCorner.Y = area.UpperLeftCorner.Y - 512.0f;
        area.UpperLeftCorner.X = area.UpperLeftCorner.X + (section % sectionsX) * 512.0f - width * 0.5f + 256.0f;
        area.LowerRightCorner.X = area.UpperLeftCorner.X + width;
        break;
    case 1:
        area.LowerRightCorner.X = area.UpperLeftCorner.X - 50.0f;
        area.UpperLeftCorner.X = area.UpperLeftCorner.X - 512.0f;
        area.UpperLeftCorner.Y = area.UpperLeftCorner.Y + (section % sectionsY) * 512.0f - width * 0.5f + 256.0f;
        area.LowerRightCorner.Y = area.UpperLeftCorner.Y + width;
        break;
    case 2:
        area.UpperLeftCorner.Y = area.LowerRightCorner.Y + 50.0f;
        area.LowerRightCorner.Y = area.LowerRightCorner.Y + 512.0f;
        area.UpperLeftCorner.X = area.UpperLeftCorner.X + (section % sectionsX) * 512.0f - width * 0.5f + 256.0f;
        area.LowerRightCorner.X = area.UpperLeftCorner.X + width;
        break;
    case 3:
        area.UpperLeftCorner.X = area.LowerRightCorner.X + 50.0f;
        area.LowerRightCorner.X = area.LowerRightCorner.X + 512.0f;
        area.UpperLeftCorner.Y = area.UpperLeftCorner.Y + (section % sectionsY) * 512.0f - width * 0.5f + 256.0f;
        area.LowerRightCorner.Y = area.UpperLeftCorner.Y + width;
        break;
    }
}

void CThreatLevelLogic::spawnAliensInArea(const ox::core::CRect<float>& area1,
    const ox::core::CRect<float>& area2, int count, int attack)
{
    int aliens[14];
    for (int i = 0; i < 14; i++)
        aliens[i] = 0;

    aliens[0] = count;

    switch (gp_world->getPlanet())
    {
    case 0:
        if (attack % 2 == 0)
        {
            aliens[5] = ox::core::clamp((count - 13) / 5, 0, 15);
            aliens[3] = ox::core::clamp((count - 38) / 4, 0, 100);
            aliens[1] = ox::core::clamp((count - 43) / 4, 0, 1000);
            aliens[8] = ox::core::clamp((count - 70) / 10, 0, 100);

            if (count > 100)
                aliens[5] += ox::core::clamp((count - 100) / 2, 0, 100);
            if (count > 150)
                aliens[2] += ox::core::clamp(count - 140, 0, 1000);
        }
        else
        {
            aliens[5] = ox::core::clamp((count - 7) / 2, 0, 15);
            aliens[3] = ox::core::clamp((count - 31) / 4, 0, 300);
            aliens[1] = ox::core::clamp((count - 43) / 2, 0, 1000);
            aliens[8] = ox::core::clamp((count - 54) / 8, 0, 1000);
        }

        switch (count)
        {
        case 13:
            aliens[2] = 6;
            break;
        case 23:
            aliens[2] = 12;
            break;
        case 43:
            aliens[2] = 24;
            break;
        }
        break;

    case 1:
        if (attack & 1)
        {
            aliens[4] = ox::core::clamp((count - 13) / 2, 0, 15);
            aliens[3] = ox::core::clamp((count - 31) / 4, 0, 150);
            aliens[1] = ox::core::clamp((count - 43) / 2, 0, 100);
        }
        else
        {
            aliens[4] = ox::core::clamp((count - 13) / 5, 0, 150);
            aliens[3] = ox::core::clamp((count - 38) / 4, 0, 100);
            aliens[1] = ox::core::clamp((count - 43) / 4, 0, 1000);
        }

        if (count % 11 == 0 && count > 20)
            aliens[7] = count / 11;
        break;

    case 2:
        aliens[8] = (count & 1) ? ox::core::clamp((count - 25) / 8, 0, 500) : 0;
        aliens[1] = ox::core::clamp((count - 20) / 3, 0, 100);
        aliens[4] = ox::core::clamp((count - 38) / 4, 0, 1000);
        aliens[6] = (count > 50 && count % 3 == 0) ? count / 25 : 0;

        if (count == 13)
            aliens[8] = 1;
        else if (count == 22)
            aliens[4] = 2;
        break;
    }

    for (int i = 0; i < count; i++)
    {
        // the rarest types spawn first
        int alienType = 0;
        for (int j = 13; j >= 0; j--)
        {
            if (aliens[j] > 0)
            {
                aliens[j]--;
                alienType = j;
                break;
            }
        }

        if (ox::algo::CRand::rand() & 1)
            spawnAlien(area1, alienType);
        else
            spawnAlien(area2, alienType);
    }

    if (attack == 0)
    {
        ox::event::SEvent event;
        event.EventType = ox::event::EET_USER_EVENT;
        event.UserEvent.UserData1 = ECE_ATTACK_STARTED;
        event.UserEvent.UserData2 = 0;
        ox::event::gp_subscriberList->OnEvent(event);
    }
}

void CThreatLevelLogic::spawnAlien(const ox::core::CRect<float>& area, int alienType)
{
    int width = (int)(area.LowerRightCorner.X - area.UpperLeftCorner.X);
    int height = (int)(area.LowerRightCorner.Y - area.UpperLeftCorner.Y);

    float x, y;
    do
    {
        x = area.UpperLeftCorner.X + ox::algo::CRand::rand() % width;
        y = area.UpperLeftCorner.Y + ox::algo::CRand::rand() % height;
    }
    while (!gp_world->mayPlaceObjectHere(ox::core::CPosition2d<float>(x, y), false));

    entity::gp_entityManager->appendEntity(new entity::CAlienEntity(x, y, alienType), 1);
}

CThreatLevelNormal::CThreatLevelNormal()
{
}

CThreatLevelNormal::~CThreatLevelNormal()
{
}

bool CThreatLevelNormal::readGameModeSpecific(ox::io::IReadFile* file, int version)
{
    return true;
}

bool CThreatLevelNormal::writeGameModeSpecific(ox::io::IWriteFile* file)
{
    return true;
}

bool CThreatLevelNormal::update(float frameDelta)
{
    bool increased = false;

    TimeLeft -= frameDelta;
    if (TimeLeft <= 0.0f)
    {
        increaseThreatLevel();
        Direction = ox::algo::CRand::rand() % 4;
        increased = true;
    }

    int level = ThreatLevel.getValue();
    if (level > 0)
    {
        AttackTimer -= frameDelta;
        if (AttackTimer <= 0.0f)
        {
            // every tenth level and the one after it are calm, and early on only every other
            // attack comes
            if (level < 10 || level % 10 > 1)
            {
                if (level >= 10 || (AttackCount.getValue() & 1) == 0)
                {
                    // smaller attacks until the world has grown
                    if (!gp_world->hasWorldExpandedAtLeastOnce() && level < 30)
                        level = (level + 1) / 2;

                    ox::core::CRect<float> area;
                    initAttackSectionArea(Direction, SectionSeed.getValue(), ThreatLevel.getValue(), area);
                    spawnAliensInArea(area, area, level, AttackCount.getValue());
                }
            }

            AttackTimer += 15.25f;
            AttackCount.modifyValue(1);
        }
    }

    return increased;
}

CThreatLevelInsane::CThreatLevelInsane()
{
}

CThreatLevelInsane::~CThreatLevelInsane()
{
}

bool CThreatLevelInsane::readGameModeSpecific(ox::io::IReadFile* file, int version)
{
    return true;
}

bool CThreatLevelInsane::writeGameModeSpecific(ox::io::IWriteFile* file)
{
    return true;
}

bool CThreatLevelInsane::update(float frameDelta)
{
    if (ThreatLevel.getValue())
        frameDelta *= 4.0f;

    bool increased = false;

    TimeLeft -= frameDelta;
    if (TimeLeft <= 0.0f)
    {
        increaseThreatLevel();
        Direction = ox::algo::CRand::rand() % 4;
        increased = true;
    }

    int level = ThreatLevel.getValue();
    if (level > 0)
    {
        AttackTimer -= frameDelta;
        if (AttackTimer <= 0.0f)
        {
            AttackTimer += 15.25f;

            if (level < 10 || level % 10 > 1)
            {
                ox::core::CRect<float> area;
                initAttackBorderArea(Direction, area);
                spawnAliensInArea(area, area, level, AttackCount.getValue());
                AttackCount.modifyValue(1);
            }
        }
    }

    return increased;
}

CThreatLevelRush::CThreatLevelRush()
{
    TimeLeft = 120.0f;
}

CThreatLevelRush::~CThreatLevelRush()
{
}

bool CThreatLevelRush::readGameModeSpecific(ox::io::IReadFile* file, int version)
{
    return true;
}

bool CThreatLevelRush::writeGameModeSpecific(ox::io::IWriteFile* file)
{
    return true;
}

bool CThreatLevelRush::update(float frameDelta)
{
    // a single threat level: after the countdown, 51 aliens attack from two sides every 10 seconds
    if (ThreatLevel.getValue() == 0)
    {
        TimeLeft -= frameDelta;
        if (TimeLeft <= 0.0f)
        {
            TimeLeft += 60.0f;
            LevelDuration = 60.0f;
            ThreatLevel.modifyValue(1);
            AttackCount.setValue(0);
            AttackTimer = 0.0f;
            Direction = ox::algo::CRand::rand() % 4;
        }
        return false;
    }

    AttackTimer -= frameDelta;
    if (AttackTimer <= 0.0f)
    {
        AttackTimer += 10.0f;

        ox::core::CRect<float> right;
        ox::core::CRect<float> left;
        initAttackBorderArea(3, right);
        initAttackBorderArea(1, left);
        spawnAliensInArea(right, left, 51, AttackCount.getValue());
    }

    return false;
}

CThreatLevelWave::CThreatLevelWave()
    : SpawnTimer(0.0f), WaveArriving(false)
{
    TimeLeft = 540.0f;
    LevelDuration = 540.0f;
    updateWaveNames();
}

void CThreatLevelWave::updateWaveNames()
{
    int level = ThreatLevel.getValue();
    int planet = gp_world->getPlanet();

    for (int wave = 0; wave < 10; wave++)
    {
        if (hasWaveBeenLaunched(wave))
        {
            WaveNames[wave] = L"";
            continue;
        }

        WaveNames[wave] = L"#1";
        WaveNames[wave].append(settings::gp_systemConfig->getLocalizedText(L"ingame:waveNameLevel", wave + 1, level + 1));
        WaveNames[wave].append(ox::core::CString<wchar_t>(L"#o"));

        for (int alienType = 0; alienType < 14; alienType++)
        {
            int count = WAVE_DEFINITIONS[planet][wave].Aliens[alienType];
            if (count <= 0)
                continue;

            for (int i = 0; i < level; i++)
                count = (int)(count * 1.3f + 1.0f);

            // the wave attacks from all four sides
            WaveNames[wave].append(ox::core::CString<wchar_t>(L"\n"));
            WaveNames[wave].append(count * 4);
            WaveNames[wave].append(ox::core::CString<wchar_t>(L" "));
            WaveNames[wave].append(settings::gp_systemConfig->getLocalizedText(entity::ALIEN_KEY_NAMES[alienType]));
        }
    }
}

CThreatLevelWave::~CThreatLevelWave()
{
}

bool CThreatLevelWave::readGameModeSpecific(ox::io::IReadFile* file, int version)
{
    SpawnTimer = ox::io::CHelpIO::readFloat(file);

    int count = ox::io::CHelpIO::readInt(file);
    for (int i = 0; i < count; i++)
    {
        SWaitingAlien alien;
        alien.AlienType = ox::io::CHelpIO::readInt(file);
        alien.Direction = ox::io::CHelpIO::readInt(file);
        WaitingAliens.push_back(alien);
    }

    if (version >= 19)
    {
        LaunchedWaves.setValue(ox::io::CHelpIO::readInt(file));
        Reward.setValue(ox::io::CHelpIO::readInt(file));
        WaveArriving = ox::io::CHelpIO::readInt(file) != 0;
    }

    updateWaveNames();
    return true;
}

bool CThreatLevelWave::writeGameModeSpecific(ox::io::IWriteFile* file)
{
    ox::io::CHelpIO::writeFloat(file, SpawnTimer);
    ox::io::CHelpIO::writeInt(file, WaitingAliens.size());

    for (std::list<SWaitingAlien>::iterator it = WaitingAliens.begin(); it != WaitingAliens.end(); ++it)
    {
        ox::io::CHelpIO::writeInt(file, it->AlienType);
        ox::io::CHelpIO::writeInt(file, it->Direction);
    }

    ox::io::CHelpIO::writeInt(file, LaunchedWaves.getValue());
    ox::io::CHelpIO::writeInt(file, Reward.getValue());
    ox::io::CHelpIO::writeInt(file, WaveArriving);
    return true;
}

bool CThreatLevelWave::update(float frameDelta)
{
    if (WaitingAliens.empty())
        return false;

    // a queued alien arrives every 25 milliseconds, from a random place in the queue
    float time = frameDelta + SpawnTimer;
    while (time > 0.025f && !WaitingAliens.empty())
    {
        time -= 0.025f;

        int index = ox::algo::CRand::rand() % (int)WaitingAliens.size();
        std::list<SWaitingAlien>::iterator it = ox::algo::advanceIterator(WaitingAliens.begin(), index);

        ox::core::CRect<float> area;
        initAttackBorderArea(it->Direction, area);
        spawnAlien(area, it->AlienType);

        WaitingAliens.erase(it);
        WaveArriving = true;
    }
    SpawnTimer = time;

    return false;
}

bool CThreatLevelWave::hasWaveBeenLaunched(int wave)
{
    int mask = 1 << wave;
    return (LaunchedWaves.getValue() & mask) != 0;
}

bool CThreatLevelWave::alienIsPresentOnThisWave(int planet, int wave, int alienType)
{
    return WAVE_DEFINITIONS[planet][wave].Aliens[alienType] > 0;
}

bool CThreatLevelWave::spawnNextWaveAttack(int wave, ox::core::CString<wchar_t>& message)
{
    if (hasWaveBeenLaunched(wave))
    {
        message = L"Wave has already been launched!";
        return false;
    }

    int planet = gp_world->getPlanet();
    message = WAVE_DEFINITIONS[planet][wave].Name;
    message.append(ox::core::CString<wchar_t>(L" wave launched! "));

    int level = ThreatLevel.getValue();
    for (int direction = 0; direction < 4; direction++)
    {
        if (level > 9)
            continue;

        for (int alienType = 0; alienType < 14; alienType++)
        {
            int count = WAVE_DEFINITIONS[planet][wave].Aliens[alienType];
            if (count <= 0)
                continue;

            for (int i = 0; i < level; i++)
                count = (int)(count * 1.3f + 1.0f);

            for (int i = 0; i < count; i++)
            {
                SWaitingAlien alien;
                alien.AlienType = alienType;
                alien.Direction = direction;
                WaitingAliens.push_back(alien);
            }

            if (direction == 0)
            {
                message.append(count);
                message.append(ox::core::CString<wchar_t>(L" "));
                message.append(ox::core::CString<wchar_t>(entity::ALIEN_KEY_NAMES[alienType]));
                message.append(ox::core::CString<wchar_t>(L"   "));
            }
        }
    }

    Reward.modifyValue(ThreatLevel.getValue() * 250 + 500);
    WaveArriving = false;
    ThreatLevel.modifyValue(1);
    LaunchedWaves.setValue(LaunchedWaves.getValue() | (1 << wave));
    updateWaveNames();
    return true;
}

const wchar_t* CThreatLevelWave::getWaveDescription(int wave)
{
    return WaveNames[wave].c_str();
}

int CThreatLevelWave::getWaveReward()
{
    int reward = 0;
    if (WaveArriving)
    {
        reward = Reward.getValue();
        if (reward > 0)
        {
            Reward.setValue(0);
            WaveArriving = false;
        }
    }
    return reward;
}

CThreatLevelCreative::CThreatLevelCreative()
{
}

CThreatLevelCreative::~CThreatLevelCreative()
{
}

bool CThreatLevelCreative::readGameModeSpecific(ox::io::IReadFile* file, int version)
{
    return true;
}

bool CThreatLevelCreative::writeGameModeSpecific(ox::io::IWriteFile* file)
{
    return true;
}

bool CThreatLevelCreative::update(float frameDelta)
{
    return false;
}

} // end namespace game
} // end namespace harvest
