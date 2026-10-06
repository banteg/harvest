// Where the port finds the game's data and keeps the user's (see docs/port/input-and-window.md,
// "Paths and OS services").

#ifndef HARVEST_PORT_PLATFORM_PATHS_H
#define HARVEST_PORT_PLATFORM_PATHS_H

#include <string>

namespace port {

//! The per-user data directory for an application, without a trailing separator: $HOME/.<name> on
//! Linux (the original's), else SDL's preference path (~/Library/Application Support/<name> on
//! macOS, %APPDATA%\<name> on Windows).
std::string getUserDataPath(const char* application);

//! Searches for the directory that holds harvestClientData/ ($GAME_RESOURCES$), in the order
//! documented in docs/port/input-and-window.md: --data, $HARVEST_DATA, the folder given last time,
//! next to the executable, Steam libraries, the usual install folders, the user data directory.
//! On success getGameDataDirectory() returns it. Otherwise returns false with `problem` set to a
//! message for the player: what is missing, where the search looked and how to fix it.
bool findGameData(std::string& problem);

//! The directory findGameData found, without a trailing separator; empty before it ran.
const std::string& getGameDataDirectory();

} // end namespace port

#endif
