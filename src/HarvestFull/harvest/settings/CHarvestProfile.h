// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_SETTINGS_CHARVESTPROFILE_H
#define HARVEST_SETTINGS_CHARVESTPROFILE_H

#include "ox/Keycodes.h"
#include "ox/core/CString.h"

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

//! A player profile: settings, key bindings, scores and achievements.
class CHarvestProfile
{
public:
    virtual ~CHarvestProfile();

    //! Binds the key to the command. Returns false if the key cannot be bound. Mac has the key type
    //! as ox::input::EKEY_CODE.
    bool makeKeyMapping(ox::EKEY_CODE key, EKeyCommands command);
    //! The key bound to the command, or 0.
    int getKeyForCommand(EKeyCommands command);
    //! Returns the game command bound to the key.
    int getCommandForKey(ox::EKEY_CODE key);
    //! Stores a priority set in the CAlienPriorities string form.
    void setAttackPriority(int index, const ox::core::CString<wchar_t>& priorities);
    void setAttackRangeMatters(int index, bool matters);

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
};

} // end namespace settings
} // end namespace harvest

#endif
