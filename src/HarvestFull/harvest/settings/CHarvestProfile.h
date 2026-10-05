// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_SETTINGS_CHARVESTPROFILE_H
#define HARVEST_SETTINGS_CHARVESTPROFILE_H

#include "ox/core/CString.h"

namespace harvest {
namespace settings {

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

//! A player profile with its settings, scores and achievements.
class CHarvestProfile
{
public:
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
