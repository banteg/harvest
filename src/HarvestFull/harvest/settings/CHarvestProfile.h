// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Method names are from the Mac symbols; member names are ours. The Linux build has no Steam
// achievement reporting.

#ifndef HARVEST_SETTINGS_CHARVESTPROFILE_H
#define HARVEST_SETTINGS_CHARVESTPROFILE_H

#include "ox/Keycodes.h"
#include "ox/core/CString.h"

namespace ox {
namespace io { class IFileSystem; }
namespace game { class CConfiguration; }
} // end namespace ox

namespace harvest {
namespace settings {

//! The commands keys can be mapped to, named after their "commandString:" texts.
enum EKeyCommands
{
    EKC_NONE = 0,
    EKC_INCREASE_SPEED,
    EKC_DECREASE_SPEED,
    EKC_SPEED_PAUSED,
    EKC_SPEED_SLOWER,
    EKC_SPEED_NORMAL,
    EKC_SPEED_FASTER,
    EKC_SPEED_FASTEST,
    EKC_SPEED_PAUSE_TOGGLE,
    EKC_BUILD_PRODUCER,
    EKC_BUILD_MOVER,
    EKC_BUILD_MINER,
    EKC_BUILD_TOWER,
    EKC_BUILD_LAUNCHER,
    EKC_ACTION_SPECIAL,
    EKC_ACTION_EAGLE,
    EKC_ACTION_TEMPEST,
    EKC_ACTION_SELL,
    EKC_ACTION_SOMETHING,
    EKC_GAME_SETTINGS,
    EKC_GAME_PRIORITIES,
    EKC_GAME_RANGES,
    EKC_GAME_OVERHEATS
};

//! Main achievements come first, the mini achievements after them.
const int MAIN_ACHIEVEMENT_COUNT = 25;
const int MINI_ACHIEVEMENT_COUNT = 8;
const int ACHIEVEMENT_COUNT = MAIN_ACHIEVEMENT_COUNT + MINI_ACHIEVEMENT_COUNT;

//! The achievement score each rating starts at.
const int ACHIEVEMENT_RATING_LEVELS[] =
{
    0, 1, 4, 7, 11, 16, 22, 29, 37, 46, 57, 70, 84, 100, 116, 131, 149
};

//! Achievement flags: the achievement is complete, or done on a planet.
enum EAchievementFlags
{
    EAF_COMPLETE = 0x1,
    EAF_PLANET_1 = 0x2,
    EAF_PLANET_2 = 0x4,
    EAF_PLANET_3 = 0x8
};

//! A player profile: its name, local high scores, achievements and key mapping.
class CHarvestProfile
{
public:
    CHarvestProfile();
    virtual ~CHarvestProfile();

    bool openProfile(ox::io::IFileSystem* fileSystem, const ox::core::CString<char>& filename);
    bool verifyAttributes();
    const wchar_t* getPriorityAttributeName(int weapon);
    void parseLocalAchievementsString();
    void parseKeyMappingString();
    bool createNewProfile(const ox::core::CString<wchar_t>& name, ox::io::IFileSystem* fileSystem,
        const ox::core::CString<char>& filename);
    void writeProfile();
    void createLocalAchievementsString();
    void createKeyMappingString();
    ox::core::CString<wchar_t> getPlayerName();
    ox::core::CString<wchar_t> returnStringAttribute(const wchar_t* name);
    ox::core::CString<wchar_t> getPlayerGroup();
    ox::core::CString<char> getFilename();
    ox::core::CString<wchar_t> getAttackPriority(int weapon);
    bool getAttackRangeMatters(int weapon);
    void setAttackPriority(int weapon, const ox::core::CString<wchar_t>& priorities);
    void setAttackRangeMatters(int weapon, bool matters);
    void setPlayerName(const ox::core::CString<wchar_t>& name);
    void setStringAttribute(const wchar_t* name, const ox::core::CString<wchar_t>& value);
    void setPlayerGroup(const ox::core::CString<wchar_t>& group);
    int getLocalScore(int type, int level, int planet);
    void updateLocalScore(int type, int level, int planet, int score);
    bool notifyMainAchievement(int achievement, int planet);
    bool notifyMiniAchievement(int achievement, int planet);
    int getAchievementScore();
    int getAchievementRating(int score);
    bool hasMainAchievement(int achievement);
    bool hasMainAchievementAtPlanet(int achievement, int planet);
    bool hasMiniAchievement(int achievement, int planet);
    static ox::core::CString<char> getAchievementSpriteName(int achievement, int planet);
    static bool isMultiPlanetAchievement(int achievement);
    bool makeKeyMapping(ox::EKEY_CODE key, EKeyCommands command);
    EKeyCommands getCommandForKey(ox::EKEY_CODE key);
    ox::EKEY_CODE getKeyForCommand(EKeyCommands command);

private:
    ox::core::CString<char> Filename;
    ox::game::CConfiguration* Config;
    //! Set once the local scores are read; nothing sets it in 1.18.
    bool LocalScoresRead;
    //! Indexed by score type (levels, minerals, times), planet and level.
    int LocalScores[3][3][5];
    //! EAchievementFlags of each achievement.
    unsigned char Achievements[ACHIEVEMENT_COUNT];
    //! The command of each key code.
    EKeyCommands KeyMapping[255];
};

} // end namespace settings
} // end namespace harvest

#endif
