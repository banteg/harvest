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

//! The game commands that can be bound to keys.
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
    EKC_GAME_OVERHEATS,
    EKC_COUNT
};

//! Localization keys of the key commands, indexed by EKeyCommands.
static const wchar_t* const KeyCommandNames[EKC_COUNT] =
{
    L"-",
    L"commandString:increaseSpeed",
    L"commandString:decreaseSpeed",
    L"commandString:speedPaused",
    L"commandString:speedSlower",
    L"commandString:speedNormal",
    L"commandString:speedFaster",
    L"commandString:speedFastest",
    L"commandString:speedPauseToggle",
    L"commandString:buildProducer",
    L"commandString:buildMover",
    L"commandString:buildMiner",
    L"commandString:buildTower",
    L"commandString:buildLauncher",
    L"commandString:actionSpecial",
    L"commandString:actionEagle",
    L"commandString:actionTempest",
    L"commandString:actionSell",
    L"commandString:actionSomething",
    L"commandString:gameSettings",
    L"commandString:gamePriorities",
    L"commandString:gameRanges",
    L"commandString:gameOverheats"
};

//! The main achievements, each earned once.
const int ACHIEVEMENT_MAIN_COUNT = 25;
//! The mini achievements, each earned once per planet.
const int ACHIEVEMENT_MINI_COUNT = 8;

//! Text keys of the ratings that the achievement score earns.
static const wchar_t* const ACHIEVEMENT_RATING_NAMES[] =
{
    L"achievementRating:schoolGraduate",
    L"achievementRating:intern",
    L"achievementRating:distinguishedIntern",
    L"achievementRating:aspiringColonist",
    L"achievementRating:colonist",
    L"achievementRating:promisingColonist",
    L"achievementRating:chiefColonist",
    L"achievementRating:brilliantChiefColonist",
    L"achievementRating:sectionTrustee",
    L"achievementRating:directorOfSection",
    L"achievementRating:globalManager",
    L"achievementRating:executivePlanetaryBoss",
    L"achievementRating:sectorSupervisor",
    L"achievementRating:trusteeOfInterstellarOperations",
    L"achievementRating:ceo",
    L"achievementRating:chairmanOfTheBoard",
    L"achievementRating:medusaSpokesperson"
};

//! Text keys of the achievement names, the main achievements first.
static const wchar_t* const ACHIEVEMENT_NAMES[] =
{
    L"achievements:flawlessEstablishment",
    L"achievements:skilledColonisation",
    L"achievements:masterfulColonisation",
    L"achievements:supremeColonisation",
    L"achievements:ironWill",
    L"achievements:firmDefense",
    L"achievements:strategicPlanning",
    L"achievements:efficientStrategy",
    L"achievements:insaneTactics",
    L"achievements:superHumanTactics",
    L"achievements:dogfight",
    L"achievements:bombLaunch",
    L"achievements:masterIndustry",
    L"achievements:pureBlood",
    L"achievements:hero",
    L"achievements:exploration",
    L"achievements:excursion",
    L"achievements:timeIsMoney",
    L"achievements:goldenWeb",
    L"achievements:massiveEncounter",
    L"achievements:masterBlaster",
    L"achievements:humoungusLaserBeam",
    L"achievements:phoenix",
    L"achievements:perfectWave",
    L"achievements:wickedAwesome",
    L"achievements:firstHarvest",
    L"achievements:firstKill",
    L"achievements:firstExploration",
    L"achievements:firstSpark",
    L"achievements:firstBomb",
    L"achievements:firstLevels",
    L"achievements:firstWave",
    L"achievements:firstDamage"
};

//! Text keys of the achievement descriptions, in ACHIEVEMENT_NAMES order.
static const wchar_t* const ACHIEVEMENT_DESCS[] =
{
    L"achievementDescs:flawlessEstablishment",
    L"achievementDescs:skilledColonisation",
    L"achievementDescs:masterfulColonisation",
    L"achievementDescs:supremeColonisation",
    L"achievementDescs:ironWill",
    L"achievementDescs:firmDefense",
    L"achievementDescs:strategicPlanning",
    L"achievementDescs:efficientStrategy",
    L"achievementDescs:insaneTactics",
    L"achievementDescs:superHumanTactics",
    L"achievementDescs:dogfight",
    L"achievementDescs:bombLaunch",
    L"achievementDescs:masterIndustry",
    L"achievementDescs:pureBlood",
    L"achievementDescs:hero",
    L"achievementDescs:exploration",
    L"achievementDescs:excursion",
    L"achievementDescs:timeIsMoney",
    L"achievementDescs:goldenWeb",
    L"achievementDescs:massiveEncounter",
    L"achievementDescs:masterBlaster",
    L"achievementDescs:humoungusLaserBeam",
    L"achievementDescs:phoenix",
    L"achievementDescs:perfectWave",
    L"achievementDescs:wickedAwesome",
    L"achievementDescs:firstHarvest",
    L"achievementDescs:firstKill",
    L"achievementDescs:firstExploration",
    L"achievementDescs:firstSpark",
    L"achievementDescs:firstBomb",
    L"achievementDescs:firstLevels",
    L"achievementDescs:firstWave",
    L"achievementDescs:firstDamage"
};

const int ACHIEVEMENT_COUNT = ACHIEVEMENT_MAIN_COUNT + ACHIEVEMENT_MINI_COUNT;

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
    //! The ACHIEVEMENT_RATING_NAMES index that the score earns.
    int getAchievementRating(int score);
    bool hasMainAchievement(int achievement);
    bool hasMainAchievementAtPlanet(int achievement, int planet);
    bool hasMiniAchievement(int achievement, int planet);
    //! The harvestMenu.dat sprite of an achievement; mini achievements have one per planet.
    static ox::core::CString<char> getAchievementSpriteName(int achievement, int planet);
    //! True for the main achievements that are earned once on each planet.
    static bool isMultiPlanetAchievement(int achievement);
    //! Binds the key to the command. Returns false if the key was unbound. Mac has the key type
    //! as ox::input::EKEY_CODE.
    bool makeKeyMapping(ox::EKEY_CODE key, EKeyCommands command);
    EKeyCommands getCommandForKey(ox::EKEY_CODE key);
    //! The key bound to the command, or 0.
    int getKeyForCommand(EKeyCommands command);

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
