// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: the names are ours, and only the values recovered units send are listed.

#ifndef HARVEST_ECUSTOMEVENTS_H
#define HARVEST_ECUSTOMEVENTS_H

#include "ox/event/IEventReceiver.h"

namespace harvest {

//! Game events, sent as ox::event::EET_USER_EVENT with the value in UserData1.
enum ECUSTOM_EVENT
{
    //! The particle setting was changed in the settings screen.
    ECE_PARTICLE_SETTING_CHANGED = 2,
    //! The scroll speed was changed in the settings screen.
    ECE_SCROLL_SPEED_CHANGED = 3,
    //! The in-game menu was closed to continue the game.
    ECE_CONTINUE_GAME = 4,
    //! A new profile was created on the profile screen; the main menu shows its welcome dialog.
    ECE_PROFILE_CREATED = 6,
    //! The first attack of a threat level has been spawned.
    ECE_ATTACK_STARTED = 7,
    //! Leave the game for the new game selection.
    ECE_NEW_GAME = 11,
    //! Leave the main menu for the game.
    ECE_START_GAME = 12,
    //! A new language file was opened; the main menu restarts.
    ECE_LANGUAGE_CHANGED = 13,
    //! Leave the game.
    ECE_EXIT_GAME = 14,
    ECE_SHOW_SAVE_SCREEN = 17,
    ECE_SHOW_LOAD_SCREEN = 18,
    ECE_SHOW_SETTINGS_SCREEN = 19,
    ECE_SHOW_AWARDS_SCREEN = 20,
    //! A tutorial hint of the normal game mode, numbered by UserData2.
    ECE_TUTORIAL_HINT = 22,
    //! Return or space was pressed on the story screen.
    ECE_SKIP_STORY = 25,
    //! Leave the main menu for the shuttle race.
    ECE_START_SHUTTLE_RACE = 40
};

//! Sends a game event to the subscribers right away.
inline void sendCustomEvent(ECUSTOM_EVENT type)
{
    ox::event::SEvent event;
    event.EventType = ox::event::EET_USER_EVENT;
    event.UserEvent.UserData1 = type;
    event.UserEvent.UserData2 = 0;
    event.UserEvent.UserData3 = 0;
    event.UserEvent.UserPointer = 0;
    ox::event::gp_subscriberList->OnEvent(event);
}

} // end namespace harvest

#endif
