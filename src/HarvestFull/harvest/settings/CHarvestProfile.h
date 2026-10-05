// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_SETTINGS_CHARVESTPROFILE_H
#define HARVEST_SETTINGS_CHARVESTPROFILE_H

namespace harvest {
namespace settings {

//! The localization keys of the achievements, by achievement index.
static const wchar_t* const ACHIEVEMENT_NAMES[] = {
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

//! A player profile with its settings, achievements and local scores.
class CHarvestProfile
{
public:
    //! The best local score of a kind (0 highest level, 1 total minerals, 2 play time in
    //! milliseconds) for a game mode and planet.
    int getLocalScore(int type, int gameMode, int planet);
};

} // end namespace settings
} // end namespace harvest

#endif
