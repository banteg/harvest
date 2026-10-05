// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_GUI_CACHIEVEMENTSSCREEN_H
#define HARVEST_GUI_CACHIEVEMENTSSCREEN_H

#include "ox/event/IEventReceiver.h"
#include "harvest/settings/CHarvestProfile.h"

namespace ox {
class IOxDevice;
namespace gui {
class IGUIElement;
class IGUIEnvironment;
class IGUILayout;
class IGUIWindow;
} // end namespace gui
namespace video {
class ISpriteAnimationState;
class IVideoDriver;
} // end namespace video
} // end namespace ox

namespace harvest {
namespace gui {

//! The award board: the main and mini achievements of the current profile with their tooltips
//! and the Medusa dialog of an earned award.
class CAchievementsScreen : public ox::event::IEventReceiver
{
public:
    //! Every award on the board: the main achievements, then three per mini achievement.
    enum
    {
        AWARD_COUNT = settings::ACHIEVEMENT_MAIN_COUNT + settings::ACHIEVEMENT_MINI_COUNT * 3
    };

    CAchievementsScreen(ox::IOxDevice* device);
    virtual ~CAchievementsScreen();

    virtual bool OnEvent(const ox::event::SEvent& event);

    void update(float time);
    void setVisible(bool visible);
    bool isVisible();

private:
    enum
    {
        ID_AWARDS = 0x2110,
        ID_CLOSE = 0x210f,
        ID_MEDUSA_CLOSE = 0x2111
    };

    //! The tooltip of an award; planet is -1 for the main achievements.
    ox::gui::IGUIWindow* getAwardPopup(int achievement, int planet);

    ox::IOxDevice* Device;
    ox::gui::IGUIEnvironment* GUIEnvironment;
    ox::video::IVideoDriver* Driver;
    ox::video::ISpriteAnimationState* MainAwards[settings::ACHIEVEMENT_MAIN_COUNT];
    ox::video::ISpriteAnimationState* MiniAwards[settings::ACHIEVEMENT_MINI_COUNT * 3];
    //! The highlight behind the hovered award.
    ox::video::ISpriteAnimationState* Selector[1];
    ox::gui::IGUIElement* AwardAreas[AWARD_COUNT];
    bool Achieved[AWARD_COUNT];
    ox::gui::IGUIElement* Window;
    ox::gui::IGUIWindow* Frame;
    float Time;
    //! The award under the mouse, or -1.
    int Hovered;
    //! Grows the selector after the hovered award changes.
    float SelectorScale;
    ox::gui::IGUILayout* MedusaWindow;
};

} // end namespace gui
} // end namespace harvest

#endif
