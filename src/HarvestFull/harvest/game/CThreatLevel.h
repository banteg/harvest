// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_GAME_CTHREATLEVEL_H
#define HARVEST_GAME_CTHREATLEVEL_H

#include "ox/core/CHiddenInt.h"
#include "ox/core/CRect.h"
#include "ox/core/CString.h"
#include <list>

namespace ox {
namespace io {
class IReadFile;
class IWriteFile;
} // end namespace io
} // end namespace ox

namespace harvest {
namespace game {

//! Game modes, as passed to CThreatLevel. The names are ours.
enum EGAME_MODE
{
    EGM_NORMAL = 0,
    EGM_WAVE,
    EGM_INSANE,
    EGM_RUSH,
    EGM_CREATIVE,
    //! The shuttle race minigame.
    EGM_SHUTTLE_RACE
};

//! An alien queued by a wave attack.
struct SWaitingAlien
{
    int AlienType;
    //! The side of the game field it comes from, as in CThreatLevelLogic::initAttackBorderArea.
    int Direction;
};

//! The difficulty logic of one game mode: the threat level rises over time and aliens attack.
class CThreatLevelLogic
{
public:
    CThreatLevelLogic();
    virtual ~CThreatLevelLogic();

    virtual bool readGameModeSpecific(ox::io::IReadFile* file, int version) = 0;
    virtual bool writeGameModeSpecific(ox::io::IWriteFile* file) = 0;

    //! Advances the logic. Returns true when the threat level went up.
    virtual bool update(float frameDelta) = 0;

    virtual bool allowVictory() { return false; }

    bool read(ox::io::IReadFile* file, int version);
    bool write(ox::io::IWriteFile* file);

protected:
    void increaseThreatLevel();

    //! The strip just outside one side of the game field: 0 top, 1 left, 2 bottom, 3 right.
    void initAttackBorderArea(int direction, ox::core::CRect<float>& area);

    //! A 512 unit deep strip outside one side of the game field, centered on one of its 512
    //! unit sections and widening with the threat level.
    void initAttackSectionArea(int direction, int section, int threatLevel, ox::core::CRect<float>& area);

    //! Spawns count aliens, each in one of the two areas. The mix of alien types depends on the
    //! planet, the count and the attack number.
    void spawnAliensInArea(const ox::core::CRect<float>& area1, const ox::core::CRect<float>& area2,
        int count, int attack);

    void spawnAlien(const ox::core::CRect<float>& area, int alienType);

    //! Seconds until the threat level rises.
    float TimeLeft;
    //! Seconds between threat level rises.
    float LevelDuration;
    //! Seconds until the next attack.
    float AttackTimer;
    ox::core::CHiddenInt ThreatLevel;
    //! Attacks since the threat level last rose.
    ox::core::CHiddenInt AttackCount;
    //! Picks the section attacked, see initAttackSectionArea.
    ox::core::CHiddenInt SectionSeed;
    //! The side attacks come from during this threat level.
    int Direction;

    friend class CThreatLevel;
};

class CThreatLevelNormal : public CThreatLevelLogic
{
public:
    CThreatLevelNormal();
    virtual ~CThreatLevelNormal();

    virtual bool readGameModeSpecific(ox::io::IReadFile* file, int version);
    virtual bool writeGameModeSpecific(ox::io::IWriteFile* file);
    virtual bool update(float frameDelta);
};

class CThreatLevelInsane : public CThreatLevelLogic
{
public:
    CThreatLevelInsane();
    virtual ~CThreatLevelInsane();

    virtual bool readGameModeSpecific(ox::io::IReadFile* file, int version);
    virtual bool writeGameModeSpecific(ox::io::IWriteFile* file);
    virtual bool update(float frameDelta);
};

class CThreatLevelRush : public CThreatLevelLogic
{
public:
    CThreatLevelRush();
    virtual ~CThreatLevelRush();

    virtual bool readGameModeSpecific(ox::io::IReadFile* file, int version);
    virtual bool writeGameModeSpecific(ox::io::IWriteFile* file);
    virtual bool update(float frameDelta);
};

//! Ten waves per planet that the player launches; each wave queues its aliens on all four sides.
class CThreatLevelWave : public CThreatLevelLogic
{
public:
    CThreatLevelWave();
    virtual ~CThreatLevelWave();

    virtual bool readGameModeSpecific(ox::io::IReadFile* file, int version);
    virtual bool writeGameModeSpecific(ox::io::IWriteFile* file);
    virtual bool update(float frameDelta);

    //! Victory needs every queued alien to have arrived.
    virtual bool allowVictory() { return WaitingAliens.empty(); }

    bool hasWaveBeenLaunched(int wave);
    bool alienIsPresentOnThisWave(int planet, int wave, int alienType);
    bool spawnNextWaveAttack(int wave, ox::core::CString<wchar_t>& message);
    const wchar_t* getWaveDescription(int wave);

    //! The reward of the last launched wave, once its aliens start arriving; paid out once.
    int getWaveReward();

private:
    void updateWaveNames();

    std::list<SWaitingAlien> WaitingAliens;
    float SpawnTimer;
    //! One bit per launched wave.
    ox::core::CHiddenInt LaunchedWaves;
    ox::core::CHiddenInt Reward;
    bool WaveArriving;
    ox::core::CString<wchar_t> WaveNames[10];
};

class CThreatLevelCreative : public CThreatLevelLogic
{
public:
    CThreatLevelCreative();
    virtual ~CThreatLevelCreative();

    virtual bool readGameModeSpecific(ox::io::IReadFile* file, int version);
    virtual bool writeGameModeSpecific(ox::io::IWriteFile* file);
    virtual bool update(float frameDelta);
};

//! The threat level of a game, driven by the logic of its game mode.
class CThreatLevel
{
public:
    CThreatLevel(int gameMode);
    virtual ~CThreatLevel();

    int getThreatLevel();
    //! How far the current threat level is towards the next one, from 0 to 1.
    float getThreatLevelProgress();

    bool read(ox::io::IReadFile* file, int version);
    bool write(ox::io::IWriteFile* file);

    bool update(float frameDelta);

    static bool alienOccursOnPlanet(int planet, int alienType);
    bool alienIsPresentAtThisLevel(int alienType);

    bool hasWaveBeenLaunched(int wave);
    bool alienIsPresentOnThisWave(int planet, int wave, int alienType);
    bool spawnNextWaveAttack(int wave, ox::core::CString<wchar_t>& message);
    const wchar_t* getWaveDescription(int wave);

    bool allowVictory();
    int getWaveReward();

private:
    void createLogicForGameMode();

    CThreatLevelLogic* Logic;
    int GameMode;
};

} // end namespace game
} // end namespace harvest

#endif
