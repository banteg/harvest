# Mods

A mod is a folder holding a `main.lua` script and an optional `favicon.tga` icon. A small `.hmd`
descriptor next to the folder names it. The folder and descriptor can be loose files or packed
together in a zip archive. Mods run only in **creative mode**. Picking creative mode on the main
menu, when any mod is installed, opens a list where the player ticks the mods to run. The scripts
call into the game through the `harvest` Lua library and react to game events through hooks.

Everything here is from [`CLuaManager.cpp`](../../src/HarvestFull/harvest/game/CLuaManager.cpp)
(180 of 204 functions exact; the rest checked for behaviour against the target), the creative-mode
code in [`CMainMenuState.cpp`](../../src/HarvestFull/harvest/states/CMainMenuState.cpp) and
[`CPlayState.cpp`](../../src/HarvestFull/harvest/states/CPlayState.cpp), and the shipped files.

## The `.hmd` descriptor

The descriptor is a configuration file ([`CConfiguration.h`](../../src/ox/game/CConfiguration.h)):
UTF-16 with a byte order mark, or 8-bit text, holding one `mod` block.

```
mod
{
	name = Emulate Normal Mode;
	folder = normal;
	author = Oxeye Game Studio;
}
```

| Attribute | Required | Meaning |
|---|---|---|
| `mod:name` | yes | the name shown in the mod list |
| `mod:folder` | yes | the mod's folder, relative to the directory holding the `.hmd`. It also identifies the mod in save games |
| `mod:author` | no | shown under the name |

The `.hmd` file's own name does not matter. Values run to the `;`, so names may contain spaces.

## Shipped mods

`harvestClientData/mods/` ships seven zips, plus two Lua files used by every mod:

| Zip | `.hmd` | Folder | Name | Contents |
|---|---|---|---|---|
| `InfiniteCredits.zip` | `infiniteCredits.hmd` | `infiniteCredits/` | Infinite Credits | `main.lua`, `favicon.tga` |
| `NormalMode.zip` | `normal.hmd` | `normal/` | Emulate Normal Mode | same |
| `RushMode.zip` | `rush.hmd` | `rush/` | Emulate Rush Mode | same |
| `ShuttleRace.zip` | `shuttle.hmd` | `shuttle/` | Somewhat Like Shuttlerace | same |
| `Starhawk.zip` | `starhawk.hmd` | `starhawk/` | Starhawk v0.6 | same, plus `starhawk.dat` (a sprite package) |
| `SuperGun.zip` | `supergun.hmd` | `supergun/` | Super Gun | `main.lua`, `favicon.tga` |
| `WaveMode.zip` | `wave.hmd` | `wave/` | Emulate Wave Mode | same |

Every zip has the same layout: the `.hmd` at the archive root and the folder next to it. Entries
are stored or deflated. The icons are 32 × 32, 32-bit uncompressed TGA.

- **`harvest.lua`** is run before any mod. It sets `print = harvest.print` and
  `dofile = harvest.dofile`, defines `ALIEN_DEFAULT` = 1 to `ALIEN_MEGA` = 9 (the first nine alien
  types), loads `table.save-0.94.lua`, and defines the `hook` table.
- **`table.save-0.94.lua`** is chillcode's table serializer.

## Discovery

`CLuaManager::refreshAvailableLuaMods` (Linux `0x44f6d0`) clears the mod list and fills it in
this order. The main menu calls it on its load step 19, and loading a save calls it again.

1. `*.hmd` files in `$GAME_RESOURCES$/harvestClientData/mods/`.
2. For each `*.zip` in that directory, the `*.hmd` files inside the archive.
3. `*.hmd` files in `$HARVEST_USERDATA$/mods/` (`~/.Harvest/mods/` on Linux; see
   [save-games.md](save-games.md#where-the-files-are)).
4. For each `*.zip` there, the `*.hmd` files inside it.

`addModsFromFiles` (Linux `0x44ddd0`) then handles each descriptor:

- The base path is the directory or archive path, with `\` changed to `/` and a trailing `/`
  added. For a zip that gives, for example, `$GAME_RESOURCES$/harvestClientData/mods/NormalMode.zip/`.
- The descriptor is read and must have `mod:name` and `mod:folder`.
- The mod path becomes `<base><folder>/`.
- The mod is listed only if `<mod path>main.lua` exists. `IFileSystem::existFile(name, true)`
  looks inside archives as well; the second parameter is named `ignoreArchives` in
  [`IFileSystem.h`](../../src/ox/io/IFileSystem.h), but true means "also search archives" (Mac
  `CFileSystem::existFile` `0xc5088`).
- It has an icon if `<mod path>favicon.tga` exists, by the same test.

Nothing removes duplicates. A mod installed in both places is listed twice. Every refresh resets
all mods to unticked.

## Running

Creative mode with any listed mod opens the selection window. Its rows show the icon, name and
author, and clicking a row or its check box toggles the mod. On *Ok*, `initializeNewGame` creates
the Lua manager whenever any mod is *available*, even if none is ticked:

1. `initCommonLuaStuff` (Linux `0x448530`):
   - creates a new Lua state (LuaJIT, Lua 5.1 API) and opens **all** standard libraries,
     including `io` and `os`;
   - registers the `harvest` library and the three object classes below;
   - runs `$GAME_RESOURCES$/harvestClientData/mods/harvest.lua` with `luaL_loadfile`, from the
     alias-resolved path on disk;
   - takes a registry reference to `hook.call`, which every engine hook goes through.
2. Each ticked mod, in list order, through `includeMod` (Linux `0x44c110`):
   - reads `<mod path>main.lua` through the file system, so it works inside zips;
   - prefixes the text with the line `-- FILE@<mod path>main.lua`;
   - runs it with `luaL_loadstring` and `lua_pcall`.

   A mod that runs without error is added to the running list. Error messages are collected and,
   for a new game, shown as info lines. The extra first line shifts reported line numbers by one.
3. The `gameInit` hook.

**Script-relative paths.** `harvest.dofile`, `harvest.addSpriteState` and the creative
`setSprite` resolve file names against the calling script's folder (`extractLuaPath`). If the
chunk's source starts with the `-- FILE@` marker, the folder is the marker's path up to its last
`/`. Otherwise it is the chunk name without its leading `@`, with `\` changed to `/`, up to and
including the last `/`.

**`harvest.dofile`** (Linux `0x44a910`). If the file exists on disk (`existFile(name, false)`,
which does not look in archives), it is loaded with `luaL_loadfile` from the alias-resolved path
and run with `lua_call`, so errors propagate. Otherwise it is read through the file system (zips
included), given a `-- FILE@` marker, and run with `luaL_loadstring` and `lua_pcall`, and errors
are ignored. The return value is whatever the chunk returned.

**Hooks.** Every engine event calls `hook.call(name, args…)` through the registry reference.
`hook.call` returns 1 if no function was ever added for the name, and 0 otherwise. The engine
remembers the first answer for each hook. After a 1 it **never calls that hook again** in the game,
so a mod must `hook.add` its handlers when its `main.lua` runs, not later. Errors inside hooks are
dropped. The exception is `frameUpdate`, whose errors are appended to `luaoutput.txt` and shown as
an info line.

| Hook | Arguments | When |
|---|---|---|
| `gameInit` | — | after the mods load in a new game |
| `frameUpdate` | dt (seconds) | every game update |
| `gameSave` | t (table to fill) | when saving; see [save-games.md](save-games.md#lua-section) |
| `gameLoad` | t (the saved table) | after loading a save |
| `mouseButton` | x, y (world), pressed, button | mouse clicks in the game area |
| `alienDeath` | alien type (1-based), x, y, z | an alien dies |
| `alienPlaced` | alien | an alien spawns |
| `buildingPlaced` | building, building type id | a construction site is placed |
| `buildingCompleted` | building | construction finishes |
| `buildingDestroyed` | building type id, x, y, alien | an alien destroys a building |
| `buildingSold` | building type id, x, y, credits | a building is sold |
| `unitSelected` | id | a unit is selected |
| `creditsMined` | miner, minerals id | a miner delivers |
| `minerOutOfMinerals` | miner | a miner's deposit runs out |
| `sparkCreated` | spark id, building | a spark is emitted |
| `missileLaunched` | missile type, turret, target id or nil, x or nil, y or nil | a turret fires |
| `energyLinkOverheated`, `energyLinkCharging` | link | energy link state |
| `energyLinkCharged` | x, y | — |
| `mapExpanded` | left, top, right, bottom | the play field grows |
| `waveButton` | button (1-based) | a wave button is clicked |
| `textInput` | text | the player submits text in the game's text box |
| `<id>_Init`, `<id>_Update` | state, privates (, dt) | per creative building of type `<id>`; remembered per name |

## The `harvest` library

The table lists the 61 functions in `g_harvestSystemLib`. Arguments in brackets are optional.
Unless noted, a call with too few arguments does nothing and returns nothing. Coordinates are
world units unless they say "screen". "Building" and "alien" results are objects with the methods
listed after the table.

| Function | Arguments | What it does |
|---|---|---|
| `print` | text | shows text as an info line in the game |
| `setThreatLevelProgress` | p | sets the threat-level progress shown by the HUD (float) |
| `setThreatLevelValue` | n | sets the threat level shown (int) |
| `getCredits` | — | returns the credits (0 without a game) |
| `addCredits` | n | adds n, rounded to nearest, keeping credits in 0 to 9999999 |
| `getPlanet` | — | returns `"heph"`, `"pose"` or `"ares"` |
| `getWorldBorders` | — | returns left, top, right, bottom of the visible game field |
| `isAlienAvailable` | type | true if alien type (1-based) occurs on this planet |
| `isPositionBlocked` | x, y [, building id] | true if the world forbids building there. With a building id and a free spot, it returns `isBuildingPlacementOk` as it is, so true when the building **fits**: inverted relative to the name (Linux `0x445740`) |
| `spawnAlien` | type, x, y | spawns alien type (1-based, rounded) if it occurs on this planet |
| `spawnBuilding` | building id, x, y [, progress = 1] | places a construction site at that progress; returns its id, or −1 for an unknown id |
| `spawnChargeBomb` | x, y [, elapsed = 0] | a charge-bomb explosion whose fuse is (1 − elapsed) × 3 s |
| `spawnMissile` | x, y, tx, ty | a missile from (x, y) to (tx, ty) |
| `spawnEagleMissile` | x, y [, z = 0 [, target id]] | an eagle missile at (x, y, z). Without a target (or id ≤ 0) it targets the priority alien within 1500 units |
| `spawnTempestMissile` | x, y, tx, ty | a tempest missile |
| `spawnParticle` | name, x, y [, z = 1 [, vx, vy [, vz]]] | a particle effect from the particle package; the speed needs at least vx and vy |
| `removeMinerals` | — / id / id, amount / x, y, r / x, y, r, amount / l, t, r, b, amount | by argument count: remove all deposits; remove one; take amount from one; remove those within r; take amount from those within r; take amount from those in the rectangle |
| `spawnMinerals` | x, y, amount | adds amount to a deposit within 10 units, or creates a new one |
| `screenToWorldCoordinates` | sx, sy | returns x, y |
| `worldToScreenCoordinates` | x, y | returns sx, sy (integers) |
| `getScreenSize` | — | returns width, height |
| `drawText` | text, sx, sy [, align [, r, g, b [, a]]] | queues text for this frame (default white) |
| `drawLine` | sx1, sy1, sx2, sy2 [, r, g, b [, a]] | queues a line for this frame |
| `drawRectangle` | sx, sy, w, h [, r, g, b [, a]] | queues a filled rectangle for this frame |
| `findBuildings` | [building id] / x, y, r [, id] / l, t, r, b [, id] | returns an array of buildings, minerals excluded, by argument count: all buildings or one type; within radius r; within the rectangle. With exactly 2 arguments there is no filter |
| `findAliens` | [type] / x, y, r [, type] / l, t, r, b [, type] | the same for aliens. The type is compared with the **internal 0-based** type, unlike `spawnAlien` and `getAlienType` |
| `getBuilding` | id | the building or nil |
| `getAlien` | id | the alien or nil |
| `setCreativeListVisible`, `setRushListVisible`, `setWaveListVisible`, `setTimerVisible` | bool | shows or hides that HUD panel |
| `setRushProgress` | p | rush progress bar, clamped to 0 to 1 |
| `setTimerValue` | t | HUD timer, at least 0 |
| `setWaveButtonVisible` | button (1–10), bool | shows or hides a wave button |
| `setWaveButtonAliens` | button, type… | sets which alien types (1-based, up to 14) the button shows |
| `setWaveButtonLabel` | button, n | sets the number on a wave button |
| `showInfoMessage` | text [, name [, portrait [, sound]]] | an info line with a speaker and portrait sprite. The sound plays as a voice line (see [audio.md](audio.md)) |
| `getNumAliens`, `getNumBuildings` | — | counts |
| `winGame`, `loseGame` | — | ends the game |
| `setMinimumWorldBorders` | l, t, r, b | the smallest play field, clamped to −4096 to 5120 |
| `getTotalAlienDamage` | — | the rush-mode damage statistic |
| `dofile` | file | runs a script relative to the caller (see above) |
| `addSpriteState` | name [, package] | creates an animation state from the in-game sprite package, or from `package` (a sprite package file relative to the script) when given. Returns a handle, or −1 |
| `removeSpriteState` | handle | frees it at the next frame |
| `renderSpriteState` | handle, x, y [, z = 0 [, scale = 1 [, rotation = 0 [, a [, r, g, b]]]]] | draws the state at a world position by adding a short-lived special-effect entity to layer 4 |
| `renderSpriteStateFreeShape` | handle, x1, y1, x2, y2, x3, y3, x4, y4, y [, a [, r, g, b]] | draws it stretched over four corners; `y` orders it among the entities |
| `getViewPosition` | — | returns the camera's world x, y |
| `setViewPosition` | x, y | moves the camera (integers) |
| `isKeyPressed` | key code | the key's state from the game's key table (0–255) |
| `getMousePosition` | — | returns the screen x, y |
| `setBuildingEnabled` | building id, bool | enables or disables a building type in the build menu |
| `defineUpgrade` | entity id, upgrade id, name, cost, count, value, description | adds a special upgrade to a building type (count at least 1) |
| `defineActionButton`, `defineUpgradeButton` | entity id, command | adds an action or upgrade button to a building type |
| `spawnEnergySpark` | building id | emits a spark from that building towards a random target |
| `spawnBullet` | damage, x, y, z, tx, ty, tz | a dropship bullet |
| `getSelectedBuilding` | — | the selected building, or the first selected one that is not a mineral deposit, or nil |
| `getSelectedBuildings` | — | an array of the selected buildings, or nothing |

Object methods (`Lunar` classes, called as `obj:method(…)`):

| Class | Methods |
|---|---|
| `CBuildingLuaInfo` | `getId()`, `getPosition()` → x, y; `getBuildingType()` → id string; `getDeathParticle()`; `remove()` |
| `CAlienLuaInfo` | `getId()`, `getAlienType()` (1-based), `getPosition()` → x, y, z; `dealDamage(damage, x, y, force, weapon)` → damage, killed; `applyForce(x, y)`; `setPosition(x, y [, z])`; `getDeathParticle()`; `remove()`; `setTargetBuilding(id)` |
| `CCreativeLuaState` | `getId()`, `setRenderColorF(r, g, b [, a])` (0–1), `setProgress(p)`, `getPosition()` → x, y; `setWantsEnergy(bool)`, `getEnergyLevel()`, `setEnergyLevel(e)`, `clearEnergyLevel()`, `getDeathParticle()`, `remove()`, `setSprite(name [, package])` |

## File system

The archive-level rules (which zip records are read, how entry names are matched, stored and
deflated entries) are in [zip-archives.md](zip-archives.md). Lookups inside an archive are
case-sensitive and use the full path, so `main.lua`, `favicon.tga` and the folder named in the
`.hmd` must match the stored names exactly.

Aliases, paths into archives, file lists and their sort order are described in
[file-system.md](file-system.md).
