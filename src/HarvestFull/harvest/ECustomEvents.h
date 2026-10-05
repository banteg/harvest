// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: the names are ours, and only the values recovered units send are listed.

#ifndef HARVEST_ECUSTOMEVENTS_H
#define HARVEST_ECUSTOMEVENTS_H

#include "ox/event/IEventReceiver.h"

namespace harvest {

//! Game events, sent as ox::event::EET_USER_EVENT with the value in UserData1.
enum ECUSTOM_EVENT
{
    //! The video mode changed, so the GUI must be realigned.
    ECE_VIDEO_MODE_CHANGED = 1,
    //! The particle setting was changed in the settings screen.
    ECE_PARTICLE_SETTING_CHANGED = 2,
    //! The scroll speed was changed in the settings screen.
    ECE_SCROLL_SPEED_CHANGED = 3,
    //! The in-game menu was closed to continue the game.
    ECE_CONTINUE_GAME = 4,
    //! Save the game to the save slot selected in the save screen.
    ECE_SAVE_GAME = 5,
    //! A new profile was created on the profile screen.
    ECE_PROFILE_CREATED = 6,
    //! The first attack of a threat level has been spawned.
    ECE_ATTACK_STARTED = 7,
    //! The building at UserData2, UserData3 was attacked; it is marked on the minimap.
    ECE_BUILDING_ATTACKED = 8,
    //! New minerals appeared, so the mineral gatherers look for them.
    ECE_MINERALS_APPEARED = 9,
    //! Replaces the building UserData2 (the selection when 0) by the building id in UserPointer.
    ECE_REPLACE_BUILDING = 10,
    //! Leave the game for the new game selection.
    ECE_NEW_GAME = 11,
    //! Leave the game to load the game in g_loadGameFilename.
    ECE_LOAD_GAME = 12,
    //! A new language file was opened.
    ECE_LANGUAGE_CHANGED = 13,
    //! Leave the game.
    ECE_EXIT_GAME = 14,
    //! Sent by the F5 and F7 keys.
    ECE_QUICK_SAVE = 15,
    ECE_QUICK_LOAD = 16,
    ECE_SHOW_SAVE_SCREEN = 17,
    ECE_SHOW_LOAD_SCREEN = 18,
    ECE_SHOW_SETTINGS_SCREEN = 19,
    ECE_SHOW_AWARDS_SCREEN = 20,
    //! A main achievement, numbered by UserData2, was earned.
    ECE_MAIN_ACHIEVEMENT = 21,
    //! A tutorial hint of the normal game mode, numbered by UserData2.
    ECE_TUTORIAL_HINT = 22,
    //! A boss alien is coming.
    ECE_BOSS_WARNING = 23,
    //! Ends the initial world of the rush and campaign modes, which then grows.
    ECE_END_INITIAL_WORLD = 24,
    //! Skips to the next scenario event.
    ECE_SKIP_SCENARIO_EVENT = 25,
    ECE_GAME_WON = 26,
    ECE_GAME_LOST = 27,
    //! The view follows the entity UserData2 of the layer UserData3.
    ECE_FOLLOW_ENTITY = 28,
    //! As ECE_FOLLOW_ENTITY, but the view jumps to the entity at once.
    ECE_JUMP_TO_ENTITY = 29,
    //! Moves the view to UserData2, UserData3.
    ECE_MOVE_VIEW = 30,
    //! Shows the gui::CDialogueItemInfo in UserPointer on the story screen.
    ECE_SHOW_DIALOGUE = 31,
    //! Adds the game::SInfoLineMessage in UserPointer to the info lines.
    ECE_ADD_INFO_MESSAGE = 32,
    //! Adds the text in UserPointer to the info lines.
    ECE_ADD_INFO_TEXT = 33,
    //! Closes the story screen once its dialogue ends.
    ECE_CLOSE_DIALOGUE = 34,
    ECE_DROPSHIP_KILLED_ALL_ALIENS = 35,
    ECE_DROPSHIP_LANDED = 36,
    ECE_DROPSHIP_GOING_TO_SPACE = 37,
    //! Darkens the screen except around the entities of type UserData2 in the layer UserData3.
    ECE_HIGHLIGHT_ENTITIES = 38,
    ECE_PLAY_INTRO_MUSIC = 39,
    //! Leave the game for the shuttle race.
    ECE_START_SHUTTLE_RACE = 40
};

//! Sends a game event with an optional value to the subscribers right away.
inline void sendCustomEvent(ECUSTOM_EVENT type, int value = 0)
{
    ox::event::SEvent event;
    event.EventType = ox::event::EET_USER_EVENT;
    event.UserEvent.UserData1 = type;
    event.UserEvent.UserData2 = value;
    event.UserEvent.UserData3 = 0;
    event.UserEvent.UserPointer = 0;
    ox::event::gp_subscriberList->OnEvent(event);
}

} // end namespace harvest

#endif
