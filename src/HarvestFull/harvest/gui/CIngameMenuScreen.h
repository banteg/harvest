// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef HARVEST_GUI_CINGAMEMENUSCREEN_H
#define HARVEST_GUI_CINGAMEMENUSCREEN_H

#include "ox/event/IEventReceiver.h"

namespace ox {
class IOxDevice;
namespace gui {
class IGUIEnvironment;
class IGUILayout;
} // end namespace gui
namespace video { class IVideoDriver; }
} // end namespace ox

namespace harvest {
namespace gui {

//! The menu opened with Escape during a game.
class CIngameMenuScreen : public ox::event::IEventReceiver
{
public:
    CIngameMenuScreen(ox::IOxDevice* device);
    virtual ~CIngameMenuScreen();

    virtual bool OnEvent(const ox::event::SEvent& event);

    //! Shows or hides the menu; save is disabled in game mode 5. With ingame set, new game and
    //! exit ask before abandoning the game.
    void setVisible(bool visible, bool ingame, int gameMode);
    bool isVisible();

private:
    enum
    {
        ID_NEW_GAME = 93333,
        ID_LOAD,
        ID_SAVE,
        ID_SETTINGS,
        ID_AWARDS,
        ID_EXIT,
        ID_CONTINUE,
        ID_CONFIRM_EXIT = 93341,
        ID_CONFIRM_NEW_GAME
    };

    ox::IOxDevice* Device;
    ox::gui::IGUIEnvironment* GUIEnvironment;
    ox::video::IVideoDriver* Driver;
    ox::gui::IGUILayout* Window;
    bool Ingame;
};

} // end namespace gui
} // end namespace harvest

#endif
