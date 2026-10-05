// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef HARVEST_SETTINGS_CHARVESTPROFILE_H
#define HARVEST_SETTINGS_CHARVESTPROFILE_H

namespace harvest {
namespace settings {

//! Localization keys of the achievement ratings, from the lowest.
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

//! A player profile with its settings, scores and achievements.
class CHarvestProfile
{
public:
    //! The sum of the achievement points.
    int getAchievementScore();
    //! The index of the rating the score reaches, in ACHIEVEMENT_RATING_NAMES.
    int getAchievementRating(int score);
};

} // end namespace settings
} // end namespace harvest

#endif
