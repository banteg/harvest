// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include <iostream>
#include <wchar.h>
#include "harvest/game/CWorld.h"
#include "harvest/entity/CHarvestEntity.h"
#include "CHarvestProfile.h"
#include "ox/algo/CBase64url.h"
#include "ox/core/CStringFunctions.h"
#include "ox/game/CConfiguration.h"
#include "ox/io/CHelpIO.h"
#include "ox/io/CMemReadFile.h"
#include "ox/io/CMemWriteFile.h"

namespace harvest {
namespace settings {

//! Attribute name prefixes of the local scores, by score type; the planet is appended.
static const wchar_t* const LOCAL_SCORE_ATTRIBUTES[3] =
{
    L"localscores:levels",
    L"localscores:minerals",
    L"localscores:times"
};

//! Priorities of a new profile, by weapon.
static const wchar_t* const DEFAULT_ATTACK_PRIORITIES[5] =
{
    L"2,2,2,2,2,2,2,2,2",
    L"2,2,2,2,2,2,2,2,2",
    L"2,2,2,2,2,2,2,2,2",
    L"2,2,2,2,2,2,2,2,2",
    L"2,2,2,2,2,2,2,2,2"
};

//! Whether range matters to each weapon of a new profile.
static const bool DEFAULT_ATTACK_RANGE_MATTERS[5] = { false, false, false, true, false };

CHarvestProfile::CHarvestProfile()
    : Config(0), LocalScoresRead(false)
{
    for (int type = 0; type < 3; ++type)
        for (int planet = 0; planet < 3; ++planet)
            for (int mode = 0; mode < 5; ++mode)
                LocalScores[type][planet][mode] = 0;
    for (int i = 0; i < ACHIEVEMENT_COUNT; ++i)
        Achievements[i] = 0;
    for (int i = 0; i < 255; ++i)
        KeyMapping[i] = EKC_NONE;
}

CHarvestProfile::~CHarvestProfile()
{
    if (Config)
        delete Config;
}

bool CHarvestProfile::openProfile(ox::io::IFileSystem* fileSystem, const ox::core::CString<char>& filename)
{
    if (Config)
        delete Config;
    Config = 0;
    Config = new ox::game::CConfiguration(fileSystem);
    Filename = filename;
    Config->read(filename.c_str());
    return verifyAttributes();
}

bool CHarvestProfile::verifyAttributes()
{
    if (!Config->attributeExists(L"profile:name"))
        return false;

    ox::core::CString<wchar_t> value;
    Config->getAttribute(ox::core::CString<wchar_t>(L"profile:name"), value);
    if (value.size() > 16)
        Config->setAttribute(ox::core::CString<wchar_t>(L"profile:name"), value.subString(0, 16));
    Config->getAttribute(ox::core::CString<wchar_t>(L"profile:group"), value);
    if (value.size() > 16)
        Config->setAttribute(ox::core::CString<wchar_t>(L"profile:group"), value.subString(0, 16));

    for (int i = 0; i < 5; ++i)
    {
        ox::core::CString<wchar_t> name = getPriorityAttributeName(i);
        if (!Config->attributeExists(name))
            Config->setAttribute(name, ox::core::CString<wchar_t>(DEFAULT_ATTACK_PRIORITIES[i]));
        name.append(ox::core::CString<wchar_t>(L"Dist"));
        if (!Config->attributeExists(name))
            Config->setAttribute(name, ox::core::CString<wchar_t>(DEFAULT_ATTACK_RANGE_MATTERS[i] ? L"1" : L"0"));
    }

    parseLocalAchievementsString();
    parseKeyMappingString();
    return true;
}

const wchar_t* CHarvestProfile::getPriorityAttributeName(int weapon)
{
    switch (weapon)
    {
    case 1:
        return L"prio:linkedLasers";
    case 2:
        return L"prio:missiles";
    case 3:
        return L"prio:eagles";
    case 4:
        return L"prio:tempest";
    default:
        return L"prio:lasers";
    }
}

void CHarvestProfile::parseLocalAchievementsString()
{
    if (Config && Config->attributeExists(L"localscores:achievements"))
    {
        ox::core::CString<wchar_t> value;
        Config->getAttribute(L"localscores:achievements", value);
        ox::io::CMemWriteFile* data = new ox::io::CMemWriteFile();
        ox::algo::CBase64url::decode(data, ox::core::CString<char>(value.c_str()));
        if (data->getSize() > 0)
        {
            char* buffer = data->getData();
            int size = data->getSize();
            ox::io::CMemReadFile* file = new ox::io::CMemReadFile(buffer, size, false);
            ox::io::CHelpIO::readInt(file);
            for (int i = 0; i < ACHIEVEMENT_COUNT; ++i)
            {
                Achievements[i] = ox::io::CHelpIO::readByte(file);
                if (Achievements[i] & 0xf0)
                    Achievements[i] = 0;
            }
            delete file;
        }
        delete data;
    }
}

void CHarvestProfile::parseKeyMappingString()
{
    if (!Config)
        return;

    ox::core::CString<wchar_t> value;
    if (Config->attributeExists(L"settings:keyboard"))
        Config->getAttribute(L"settings:keyboard", value);

    KeyMapping[ox::KEY_ADD] = EKC_INCREASE_SPEED;
    KeyMapping[ox::KEY_SUBTRACT] = EKC_DECREASE_SPEED;
    KeyMapping[ox::KEY_KEY_1] = EKC_SPEED_PAUSED;
    KeyMapping[ox::KEY_KEY_2] = EKC_SPEED_SLOWER;
    KeyMapping[ox::KEY_KEY_3] = EKC_SPEED_NORMAL;
    KeyMapping[ox::KEY_KEY_4] = EKC_SPEED_FASTER;
    KeyMapping[ox::KEY_KEY_5] = EKC_SPEED_FASTEST;
    KeyMapping[ox::KEY_KEY_P] = EKC_SPEED_PAUSE_TOGGLE;
    KeyMapping[ox::KEY_KEY_S] = EKC_BUILD_PRODUCER;
    KeyMapping[ox::KEY_KEY_E] = EKC_BUILD_MOVER;
    KeyMapping[ox::KEY_KEY_R] = EKC_BUILD_MINER;
    KeyMapping[ox::KEY_KEY_D] = EKC_BUILD_TOWER;
    KeyMapping[ox::KEY_KEY_T] = EKC_BUILD_LAUNCHER;
    KeyMapping[ox::KEY_KEY_X] = EKC_ACTION_SPECIAL;
    KeyMapping[ox::KEY_KEY_C] = EKC_ACTION_EAGLE;
    KeyMapping[ox::KEY_KEY_V] = EKC_ACTION_TEMPEST;
    KeyMapping[ox::KEY_BACK] = EKC_ACTION_SELL;
    KeyMapping[ox::KEY_KEY_Z] = EKC_ACTION_SOMETHING;
    KeyMapping[ox::KEY_F2] = EKC_GAME_SETTINGS;
    KeyMapping[ox::KEY_F3] = EKC_GAME_PRIORITIES;
    KeyMapping[ox::KEY_KEY_O] = EKC_GAME_SETTINGS;
    KeyMapping[ox::KEY_KEY_I] = EKC_GAME_PRIORITIES;
    KeyMapping[ox::KEY_KEY_G] = EKC_GAME_RANGES;
    KeyMapping[ox::KEY_KEY_H] = EKC_GAME_OVERHEATS;

    ox::io::CMemWriteFile* data = new ox::io::CMemWriteFile();
    ox::algo::CBase64url::decode(data, ox::core::CString<char>(value.c_str()));
    if (data->getSize() > 0)
    {
        char* buffer = data->getData();
        int size = data->getSize();
        ox::io::CMemReadFile* file = new ox::io::CMemReadFile(buffer, size, false);
        ox::io::CHelpIO::readInt(file);
        for (int i = 0; i < 255; ++i)
        {
            if (file->getRemainingSize() <= 0)
                break;
            KeyMapping[i] = (EKeyCommands)ox::io::CHelpIO::readByte(file);
        }
        delete file;
    }
    delete data;
}

bool CHarvestProfile::createNewProfile(const ox::core::CString<wchar_t>& name, ox::io::IFileSystem* fileSystem,
    const ox::core::CString<char>& filename)
{
    Config = new ox::game::CConfiguration(fileSystem);
    Filename = filename;
    Config->setAttribute(ox::core::CString<wchar_t>(L"profile:name"), name);
    verifyAttributes();
    writeProfile();
    return true;
}

void CHarvestProfile::writeProfile()
{
    if (!Config)
        return;
    createLocalAchievementsString();
    createKeyMappingString();
    Config->write(Filename.c_str());
}

void CHarvestProfile::createLocalAchievementsString()
{
    ox::io::CMemWriteFile* data = new ox::io::CMemWriteFile();
    ox::io::CHelpIO::writeInt(data, 1);
    for (int i = 0; i < ACHIEVEMENT_COUNT; ++i)
        ox::io::CHelpIO::writeByte(data, Achievements[i]);
    char* buffer = data->getData();
    int size = data->getSize();
    ox::io::CMemReadFile* file = new ox::io::CMemReadFile(buffer, size, false);
    ox::core::CString<char> encoded;
    ox::algo::CBase64url::encode(encoded, file);
    delete file;
    delete data;
    if (Config)
        Config->setAttribute(ox::core::CString<wchar_t>(L"localscores:achievements"),
            ox::core::CString<wchar_t>(encoded.c_str()));
}

void CHarvestProfile::createKeyMappingString()
{
    ox::io::CMemWriteFile* data = new ox::io::CMemWriteFile();
    ox::io::CHelpIO::writeInt(data, 1);
    for (int i = 0; i < 255; ++i)
        ox::io::CHelpIO::writeByte(data, KeyMapping[i]);
    char* buffer = data->getData();
    int size = data->getSize();
    ox::io::CMemReadFile* file = new ox::io::CMemReadFile(buffer, size, false);
    ox::core::CString<char> encoded;
    ox::algo::CBase64url::encode(encoded, file);
    delete file;
    delete data;
    if (Config)
        Config->setAttribute(ox::core::CString<wchar_t>(L"settings:keyboard"),
            ox::core::CString<wchar_t>(encoded.c_str()));
}

ox::core::CString<wchar_t> CHarvestProfile::getPlayerName()
{
    return returnStringAttribute(L"profile:name");
}

ox::core::CString<wchar_t> CHarvestProfile::returnStringAttribute(const wchar_t* name)
{
    ox::core::CString<wchar_t> value;
    if (Config)
        Config->getAttribute(name, value);
    return ox::core::CString<wchar_t>(value);
}

ox::core::CString<wchar_t> CHarvestProfile::getPlayerGroup()
{
    return returnStringAttribute(L"profile:group");
}

ox::core::CString<char> CHarvestProfile::getFilename()
{
    return Filename;
}

ox::core::CString<wchar_t> CHarvestProfile::getAttackPriority(int weapon)
{
    return returnStringAttribute(getPriorityAttributeName(weapon));
}

bool CHarvestProfile::getAttackRangeMatters(int weapon)
{
    ox::core::CString<wchar_t> name = getPriorityAttributeName(weapon);
    name.append(ox::core::CString<wchar_t>(L"Dist"));
    return Config->getAttributeAsInt(name.c_str()) != 0;
}

void CHarvestProfile::setAttackPriority(int weapon, const ox::core::CString<wchar_t>& priorities)
{
    Config->setAttribute(ox::core::CString<wchar_t>(getPriorityAttributeName(weapon)), priorities);
}

void CHarvestProfile::setAttackRangeMatters(int weapon, bool matters)
{
    ox::core::CString<wchar_t> name = getPriorityAttributeName(weapon);
    name.append(ox::core::CString<wchar_t>(L"Dist"));
    Config->setAttribute(name.c_str(), (int)matters);
}

void CHarvestProfile::setPlayerName(const ox::core::CString<wchar_t>& name)
{
    setStringAttribute(L"profile:name", name);
}

void CHarvestProfile::setStringAttribute(const wchar_t* name, const ox::core::CString<wchar_t>& value)
{
    if (Config)
        Config->setAttribute(ox::core::CString<wchar_t>(name), value);
}

void CHarvestProfile::setPlayerGroup(const ox::core::CString<wchar_t>& group)
{
    setStringAttribute(L"profile:group", group);
}

int CHarvestProfile::getLocalScore(int type, int mode, int planet)
{
    if (!LocalScoresRead)
    {
        for (int i = 0; i < 3; ++i)
        {
            for (int p = 0; p < 3; ++p)
            {
                ox::core::CString<wchar_t> name = LOCAL_SCORE_ATTRIBUTES[i];
                name.append(p);
                ox::core::CString<wchar_t> value = returnStringAttribute(name.c_str());
                ox::TArray<ox::core::CString<wchar_t> > scores;
                ox::core::splitString(scores, value, ox::core::CString<wchar_t>(L","));
                for (unsigned int m = 0; m < scores.size(); ++m)
                {
                    if (m >= 5)
                        break;
                    LocalScores[i][p][m] = wcstol(scores[m].c_str(), 0, 10);
                }
            }
        }
    }
    return LocalScores[type][planet][mode];
}

void CHarvestProfile::updateLocalScore(int type, int mode, int planet, int score)
{
    if (!LocalScoresRead)
        getLocalScore(type, planet, mode);
    LocalScores[type][planet][mode] = score;

    ox::core::CString<wchar_t> value(LocalScores[type][planet][0]);
    for (int m = 1; m < 5; ++m)
    {
        value.append(ox::core::CString<wchar_t>(L","));
        value.append(LocalScores[type][planet][m]);
    }
    ox::core::CString<wchar_t> name = LOCAL_SCORE_ATTRIBUTES[type];
    name.append(planet);
    setStringAttribute(name.c_str(), value);
}

bool CHarvestProfile::notifyMainAchievement(int achievement, int planet)
{
    unsigned char flags = Achievements[achievement];
    if ((flags & EAF_COMPLETE) || achievement >= MAIN_ACHIEVEMENT_COUNT)
        return false;

    switch (planet)
    {
    case 0:
        flags |= EAF_PLANET_1;
        Achievements[achievement] = flags;
        break;
    case 1:
        flags |= EAF_PLANET_2;
        Achievements[achievement] = flags;
        break;
    case 2:
        flags |= EAF_PLANET_3;
        Achievements[achievement] = flags;
        break;
    }

    if (isMultiPlanetAchievement(achievement))
    {
        if ((flags & (EAF_PLANET_1 | EAF_PLANET_2 | EAF_PLANET_3)) == (EAF_PLANET_1 | EAF_PLANET_2 | EAF_PLANET_3))
        {
            Achievements[achievement] = flags | EAF_COMPLETE;
            return true;
        }
        return false;
    }
    Achievements[achievement] = flags | EAF_COMPLETE;
    return true;
}

bool CHarvestProfile::notifyMiniAchievement(int achievement, int planet)
{
    unsigned char flags = Achievements[achievement];
    if ((flags & EAF_COMPLETE) || achievement < MAIN_ACHIEVEMENT_COUNT)
        return false;

    unsigned char flag = 0;
    switch (planet)
    {
    case 0:
        flag = EAF_PLANET_1;
        break;
    case 1:
        flag = EAF_PLANET_2;
        break;
    case 2:
        flag = EAF_PLANET_3;
        break;
    }
    if (flag & flags)
        return false;

    flags |= flag;
    Achievements[achievement] = flags;
    if ((flags & (EAF_PLANET_1 | EAF_PLANET_2 | EAF_PLANET_3)) == (EAF_PLANET_1 | EAF_PLANET_2 | EAF_PLANET_3))
        Achievements[achievement] = flags | EAF_COMPLETE;
    return true;
}

int CHarvestProfile::getAchievementScore()
{
    int score = 0;
    for (int i = 0; i < MAIN_ACHIEVEMENT_COUNT; ++i)
    {
        if (Achievements[i] & EAF_COMPLETE)
            score += 5;
    }
    for (int i = MAIN_ACHIEVEMENT_COUNT; i < ACHIEVEMENT_COUNT; ++i)
    {
        unsigned char flags = Achievements[i];
        if (flags & EAF_COMPLETE)
            score += 3;
        else
        {
            if (flags & EAF_PLANET_1)
                ++score;
            if (flags & EAF_PLANET_2)
                ++score;
            if (flags & EAF_PLANET_3)
                ++score;
        }
    }
    return score;
}

int CHarvestProfile::getAchievementRating(int score)
{
    for (int i = 1; i < 17; ++i)
    {
        if (score < ACHIEVEMENT_RATING_LEVELS[i])
            return i - 1;
    }
    return 16;
}

bool CHarvestProfile::hasMainAchievement(int achievement)
{
    return Achievements[achievement] & EAF_COMPLETE;
}

bool CHarvestProfile::hasMainAchievementAtPlanet(int achievement, int planet)
{
    unsigned char flag = 0;
    switch (planet)
    {
    case 0:
        flag = EAF_PLANET_1;
        break;
    case 1:
        flag = EAF_PLANET_2;
        break;
    case 2:
        flag = EAF_PLANET_3;
        break;
    }
    if (Achievements[achievement] & flag)
        return true;
    return false;
}

bool CHarvestProfile::hasMiniAchievement(int achievement, int planet)
{
    unsigned char flag = 0;
    switch (planet)
    {
    case 0:
        flag = EAF_PLANET_1;
        break;
    case 1:
        flag = EAF_PLANET_2;
        break;
    case 2:
        flag = EAF_PLANET_3;
        break;
    }
    if (Achievements[achievement] & flag)
        return true;
    return false;
}

ox::core::CString<char> CHarvestProfile::getAchievementSpriteName(int achievement, int planet)
{
    if (achievement < MAIN_ACHIEVEMENT_COUNT)
    {
        int number = achievement + 1;
        ox::core::CString<char> name = "Achievement";
        if (number < 10)
            name.append(ox::core::CString<char>("0"));
        name.append(number);
        return ox::core::CString<char>(name);
    }

    int number = achievement - (MAIN_ACHIEVEMENT_COUNT - 1);
    ox::core::CString<char> name = "MiniAchievement";
    if (number < 10)
        name.append(ox::core::CString<char>("0"));
    name.append(number);
    switch (planet)
    {
    case 0:
        name.append(ox::core::CString<char>("a"));
        break;
    case 1:
        name.append(ox::core::CString<char>("b"));
        break;
    case 2:
        name.append(ox::core::CString<char>("c"));
        break;
    }
    return ox::core::CString<char>(name);
}

bool CHarvestProfile::isMultiPlanetAchievement(int achievement)
{
    return achievement == 3 || achievement == 5 || achievement == 9 || achievement == 16;
}

bool CHarvestProfile::makeKeyMapping(ox::EKEY_CODE key, EKeyCommands command)
{
    EKeyCommands previous = KeyMapping[key];
    for (int i = 0; i < 255; ++i)
    {
        if (KeyMapping[i] == command)
            KeyMapping[i] = EKC_NONE;
    }
    KeyMapping[key] = command;
    return previous != EKC_NONE;
}

EKeyCommands CHarvestProfile::getCommandForKey(ox::EKEY_CODE key)
{
    return KeyMapping[key];
}

ox::EKEY_CODE CHarvestProfile::getKeyForCommand(EKeyCommands command)
{
    for (int i = 0; i < 255; ++i)
    {
        if (KeyMapping[i] == command)
            return (ox::EKEY_CODE)i;
    }
    return (ox::EKEY_CODE)0;
}

} // end namespace settings
} // end namespace harvest
