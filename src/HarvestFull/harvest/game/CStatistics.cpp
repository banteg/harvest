// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include "CStatistics.h"
#include "CWorld.h"
#include "harvest/entity/CHarvestEntity.h"
#include "ox/io/CHelpIO.h"
#include "ox/io/CMemReadFile.h"
#include "ox/io/CMemWriteFile.h"
#include "ox/algo/CBase64url.h"

namespace harvest {
namespace game {

CStatistics* gp_statistics = 0;
CHighscoreInfo* CHighscoreInfo::CurrentInfo = 0;

CStatistics::CStatistics() : RushModeDamage(0)
{
    Current = new SLevelStats;
    Current->Level = 0;
}

CStatistics::~CStatistics()
{
    for (unsigned int i = 0; i < Levels.size(); ++i) delete Levels[i];
    delete Current;
}

bool CStatistics::read(ox::io::IReadFile* file, int version)
{
    Current->Level = ox::io::CHelpIO::readInt(file);
    for (int i = 0; i < 7; ++i) Current->Stats[i] = ox::io::CHelpIO::readFloat(file);
    if (version >= 9)
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 14; ++j) Current->AlienStats[i][j] = ox::io::CHelpIO::readFloat(file);
    int count = ox::io::CHelpIO::readInt(file);
    for (int i = 0; i < count; ++i)
    {
        SLevelStats* level = new SLevelStats;
        level->Level = ox::io::CHelpIO::readInt(file);
        if (version >= 23) level->Time = ox::io::CHelpIO::readFloat(file);
        for (int j = 0; j < 7; ++j) level->Stats[j] = ox::io::CHelpIO::readFloat(file);
        if (version >= 9)
            for (int j = 0; j < 4; ++j)
                for (int k = 0; k < 14; ++k) level->AlienStats[j][k] = ox::io::CHelpIO::readFloat(file);
        Levels.push_back(level);
    }
    count = version >= 22 ? 7 : 6;
    for (int i = 0; i < count; ++i)
        GameStats[i].setValue(ox::io::CHelpIO::readInt(file));
    if (version >= 21) RushModeDamage = ox::io::CHelpIO::readFloat(file);
    if (version >= 24)
    {
        int count = ox::io::CHelpIO::readInt(file);
        for (int i = 0; i < count; ++i)
        {
            SEventLog log;
            log.Time = ox::io::CHelpIO::readFloat(file);
            log.Type = ox::io::CHelpIO::readInt(file);
            log.Value = ox::io::CHelpIO::readInt(file);
            Logs.push_back(log);
        }
    }
    return true;
}

bool CStatistics::write(ox::io::IWriteFile* file)
{
    ox::io::CHelpIO::writeInt(file, Current->Level);
    for (int i = 0; i < 7; ++i) ox::io::CHelpIO::writeFloat(file, Current->Stats[i]);
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 14; ++j) ox::io::CHelpIO::writeFloat(file, Current->AlienStats[i][j]);
    ox::io::CHelpIO::writeInt(file, Levels.size());
    for (unsigned int i = 0; i < Levels.size(); ++i)
    {
        ox::io::CHelpIO::writeInt(file, Levels[i]->Level);
        ox::io::CHelpIO::writeFloat(file, Levels[i]->Time);
        for (int j = 0; j < 7; ++j) ox::io::CHelpIO::writeFloat(file, Levels[i]->Stats[j]);
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 14; ++k) ox::io::CHelpIO::writeFloat(file, Levels[i]->AlienStats[j][k]);
    }
    for (int i = 0; i < 7; ++i) ox::io::CHelpIO::writeInt(file, GameStats[i].getValue());
    ox::io::CHelpIO::writeFloat(file, RushModeDamage);
    ox::io::CHelpIO::writeInt(file, Logs.size());
    for (int i = 0; i < (int)Logs.size(); ++i)
    {
        ox::io::CHelpIO::writeFloat(file, Logs[i].Time);
        ox::io::CHelpIO::writeInt(file, Logs[i].Type);
        ox::io::CHelpIO::writeInt(file, Logs[i].Value);
    }
    return true;
}

void CStatistics::reportNewThreatLevel(int level, float time)
{
    if (Current)
    {
        for (int i = 0; i < 14; ++i)
        {
            Current->Stats[3] += Current->AlienStats[0][i];
            Current->Stats[4] += Current->AlienStats[1][i];
            Current->Stats[5] += Current->AlienStats[2][i];
            Current->Stats[6] += Current->AlienStats[3][i];
        }
        GameStats[4].modifyValue((int)(Current->Stats[4] + .5f));
        GameStats[5].modifyValue((int)(Current->Stats[5] + .5f));
        GameStats[6].modifyValue((int)(Current->Stats[6] + .5f));
    }
    if (Current && Current->Level != 0)
    {
        GameStats[0].setValue(Current->Level);
        Current->Time = time;
        Levels.push_back(Current);
    }
    else delete Current;
    Current = new SLevelStats;
    Current->Level = level;
}

void CStatistics::modifyLevelStatValue(int stat, float delta)
{
    if (Current) Current->Stats[stat] += delta;
    if (stat == 2) GameStats[1].modifyValue((int)(delta + .5f));
}

void CStatistics::reportAlienStatChange(int stat, int alienType, float amount)
{
    if (Current) Current->AlienStats[stat][alienType] += amount;
    if (stat == 1 || stat == 3 || stat == 2) RushModeDamage += amount;
    if (stat == 0) GameStats[2].modifyValue((int)(amount + .5f));
}

float CStatistics::getRushModeDamage() { return RushModeDamage; }
void CStatistics::setHighest(int stat, float value)
{
    if (Current && value > Current->Stats[stat]) Current->Stats[stat] = value;
}
void CStatistics::modifyGameStatValue(int stat, int delta) { GameStats[stat].modifyValue(delta); }
const ox::TArray<SLevelStats*>& CStatistics::getAllLevelStats() { return Levels; }
void CStatistics::addLog(float time, int type, int value)
{
    SEventLog log = { time, type, value };
    Logs.push_back(log);
}
const ox::TArray<SEventLog>& CStatistics::getLogs() { return Logs; }
int CStatistics::getGameStatValue(int stat) { return GameStats[stat].getValue(); }

CHighscoreInfo::CHighscoreInfo() {}
CHighscoreInfo::CHighscoreInfo(const wchar_t* name, const wchar_t* group, int random, int startTime,
    int mode, int planet, int minerals, int level, float playTime)
    : PlayerName(name), PlayerGroup(group), RandomValue(random), StartPlayTime(startTime),
      GameMode(mode), GamePlanet(planet), TotalMinerals(minerals), HighestLevel(level), PlayTime(playTime)
{
    createHighscoreString();
}

//! XORs data with a key that rotates right every four bytes. Inlined in both builds; the name is ours.
static inline void scramble(char* data, int size, int key)
{
    for (int i = 0; i < size; ++i)
    {
        switch (i & 3)
        {
        case 0: data[i] ^= (unsigned int)key >> 24; break;
        case 1: data[i] ^= (unsigned int)key >> 16; break;
        case 2: data[i] ^= (key & 0xff00) >> 8; break;
        case 3: data[i] ^= key; key = (int)(((unsigned int)key >> 1) | ((key & 1) << 31)); break;
        }
    }
}

void CHighscoreInfo::createHighscoreString()
{
    ox::io::CMemWriteFile* file = new ox::io::CMemWriteFile;
    ox::io::CHelpIO::writeInt(file, RandomValue);
    ox::io::CHelpIO::writeInt(file, StartPlayTime);
    ox::io::CHelpIO::writeWideString(file, PlayerName, true);
    ox::io::CHelpIO::writeWideString(file, PlayerGroup, true);
    ox::io::CHelpIO::writeByte(file, GameMode);
    ox::io::CHelpIO::writeByte(file, GamePlanet);
    ox::io::CHelpIO::writeInt(file, TotalMinerals.getValue());
    ox::io::CHelpIO::writeShort(file, HighestLevel.getValue());
    ox::io::CHelpIO::writeInt(file, (int)(PlayTime * 100));

    scramble(file->getData() + 4, file->getSize() - 4, RandomValue);
    scramble(file->getData(), file->getSize(), 0x4f2c7b19);
    ox::io::CMemReadFile* reader = new ox::io::CMemReadFile(file->getData(), file->getSize(), false);
    ox::algo::CBase64url::encode(HighscoreString, reader);
    delete reader;
    delete file;
}

CHighscoreInfo::~CHighscoreInfo() {}
const wchar_t* CHighscoreInfo::getPlayerName() const { return PlayerName.c_str(); }
const wchar_t* CHighscoreInfo::getPlayerGroup() const { return PlayerGroup.c_str(); }
int CHighscoreInfo::getRandomValue() const { return RandomValue; }
int CHighscoreInfo::getStartPlayTime() const { return StartPlayTime; }
int CHighscoreInfo::getGameMode() const { return GameMode; }
int CHighscoreInfo::getGamePlanet() const { return GamePlanet; }
int CHighscoreInfo::getTotalMinerals() { return TotalMinerals.getValue(); }
int CHighscoreInfo::getHighestLevel() { return HighestLevel.getValue(); }
float CHighscoreInfo::getPlayTime() const { return PlayTime; }
const char* CHighscoreInfo::getHighscoreString() const { return HighscoreString.c_str(); }
void CHighscoreInfo::setNewHighscoreInfo(CHighscoreInfo* info)
{
    if (CurrentInfo) delete CurrentInfo;
    CurrentInfo = info;
}
CHighscoreInfo* CHighscoreInfo::getCurrentHighscoreInfo() { return CurrentInfo; }

} // end namespace game
} // end namespace harvest
