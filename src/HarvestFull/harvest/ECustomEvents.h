// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: the names are ours, and only the values recovered units send are listed.

#ifndef HARVEST_ECUSTOMEVENTS_H
#define HARVEST_ECUSTOMEVENTS_H

#include "ox/event/IEventReceiver.h"

namespace harvest {

//! Game events, sent as ox::event::EET_USER_EVENT with the value in UserData1.
enum ECUSTOM_EVENT
{
    //! The in-game menu was closed to continue the game.
    ECE_CONTINUE_GAME = 4,
    //! Show the welcome dialog of the main menu.
    ECE_SHOW_WELCOME_DIALOG = 6,
    //! The first attack of a threat level has been spawned.
    ECE_ATTACK_STARTED = 7,
    //! Leave the game for the new game selection.
    ECE_NEW_GAME = 11,
    //! Leave the main menu for the game.
    ECE_START_GAME = 12,
    //! Restart the main menu state.
    ECE_RESTART_MAIN_MENU = 13,
    //! Leave the game.
    ECE_EXIT_GAME = 14,
    ECE_SHOW_SAVE_SCREEN = 17,
    ECE_SHOW_LOAD_SCREEN = 18,
    ECE_SHOW_SETTINGS_SCREEN = 19,
    ECE_SHOW_AWARDS_SCREEN = 20,
    //! A tutorial hint of the normal game mode, numbered by UserData2.
    ECE_TUTORIAL_HINT = 22,
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
