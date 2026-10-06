Harvest: Massive Encounter, the port
====================================

A modern build of Harvest: Massive Encounter (Oxeye Game Studio, 2008) for macOS, Linux and
Windows, compiled from the game's recovered source code: https://github.com/banteg/harvest

This package holds only the port's program. It contains no game data: the game needs the
harvestClientData folder from your own copy of the original game (the Steam release or the
DRM-free one).


Game data
---------

The game looks for a folder that holds harvestClientData, in this order, and uses the first one:

 1. the folder given with --data <folder> on the command line;
 2. the folder in the HARVEST_DATA environment variable;
 3. the folder given with --data or HARVEST_DATA last time (it is remembered);
 4. the folder of the program and the folder above it; for the macOS app, the app's
    Contents/Resources and the folder the app is in;
 5. Steam: steamapps/common/Harvest Massive Encounter in every Steam library;
 6. on macOS any app in /Applications or ~/Applications that holds the data, such as the
    original Harvest.app; on Linux ~/Games/Harvest Massive Encounter and ~/Games/Harvest;
 7. the user data folder (below).

Any of these may also be (or contain) a Mac app bundle with the data in Contents/Resources, so
--data can point at the original game's "Harvest Steam.app".

The easiest setup without Steam: copy harvestClientData next to the program (on macOS, next
to the app). When nothing is found the game says where it looked and exits.


Saves and settings
------------------

  Linux    ~/.Harvest (the original's folder, so its profiles and saves carry over)
  macOS    ~/Library/Application Support/Harvest
  Windows  %APPDATA%\Harvest


Options
-------

  --data <folder>   the folder holding harvestClientData
  --scale <factor>  screen pixels per game pixel (default: the display's scale)
  --no-vsync        present frames as fast as possible
  --no-audio        play no sound
  --help            all options

Ctrl+F (Cmd+F on macOS) toggles fullscreen.


Licences
--------

The licences of the libraries built into the program (SDL, Lua, zlib, miniaudio, stb_image)
are in the licenses folder.
