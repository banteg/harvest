# Save games and profiles

A save game is a small plain header followed by the game state. The state is compressed with zlib
and then encrypted with AES-256 under a fixed key. Profiles and settings are separate text files in
the configuration format. Everything on this page comes from recovered source that matches the
Linux target (exact or behaviour-checked; see [`docs/matching.md`](../matching.md)), plus the
addresses given.

## Where the files are

| Path | Contents |
|---|---|
| `$HARVEST_USERDATA$/harvest.cfg` | system settings ([`CSystemConfig`](../../src/HarvestFull/harvest/settings/CSystemConfig.cpp)) |
| `$HARVEST_USERDATA$/profiles/Profile-YYMMDD-NN.cfg` | one file per player profile |
| `$HARVEST_USERDATA$/profiles/saveGameNN.hsg` | save games, all profiles together |
| `$HARVEST_USERDATA$/screenshots/` | Ctrl+T JPEG screenshots |
| `$HARVEST_USERDATA$/sandbox/`, `$HARVEST_USERDATA$/mods/` | user creative-building definitions and mods |

`CHarvestFullMain::init` ([`CHarvestFullMain.cpp`](../../src/HarvestFull/harvest/CHarvestFullMain.cpp))
binds `$HARVEST_USERDATA$` to `IOSOperator::getApplicationSupportPath("Harvest")` and creates the
directory and its `profiles`, `screenshots` and `sandbox` subdirectories if they are missing. On
Linux, `CLinuxOperator::getApplicationSupportPath` (`0x4c0860`) returns
`getenv("HOME") + "/." + name`, so the directory is **`~/.Harvest`**.

### Naming

- **Save games.** A new save is named by
  `CHelpIO::getNextFreeFilename("$HARVEST_USERDATA$/profiles/saveGame", ".hsg")` (Linux
  `0x5eba90`): the first n = 0, 1, 2 … for which the file does not exist, written with a leading
  `0` when n < 10. That gives `saveGame00.hsg` … `saveGame09.hsg`, then `saveGame10.hsg`.
  Overwriting a slot reuses its file name.
- **Save list.** `CSaveGameScreen::loadSaveGames` (Linux `0x47c9d0`) lists every `*.hsg` in
  `profiles/`, reads each header, and keeps those whose `PlayerName` equals the current profile's
  name. The list is sorted by `Time`, newest first. A save with an empty description shows its
  file name.
- **Profiles.** `CProfileManager::createProfile` (Linux `0x490610`) names a new profile
  `Profile-` + `strftime("%y%m%d")` + `-0n` or `-n` + `.cfg`, with the first n from 0 that is free.
  The profile list is every `*.cfg` in `profiles/` that has a `profile:name`.

## Save game file layout

All integers and floats are little-endian, 4 bytes; that is how `CHelpIO` writes native `int` and
`float` on x86 ([`CHelpIO.cpp`](../../src/ox/io/CHelpIO.cpp)). The string types:

- **wstring**: UTF-16 code units (each `wchar_t` truncated to 16 bits) ending with a 0 unit.
- **string**: bytes ending with 0.

The file is written by `CPlayState::writeStateToFile` (Linux `0x4a47e0`) and read by
`readStateFromFile` (Linux `0x4a2580`), both in
[`CPlayState.cpp`](../../src/HarvestFull/harvest/states/CPlayState.cpp).

```
header (CSavestateInfo::writeHeader, Linux 0x491b90)
  int32    magic            0x123123ff
  int32    version          30 (read: 26 to 30 accepted, others rejected)
  wstring  playerName
  int32    gameMode         0 normal, 1 wave, 2 insane, 3 rush, 4 creative, 5 campaign
  int32    threatLevel      ThreatLevel->getThreatLevel()
  int32    minerals         the credit count at the last game update (CPlayState::Minerals)
  int32    planet           0 Hephaestus, 1 Poseidon, 2 Ares
  int32    time             time(0) when saved, seconds since 1970
  wstring  description
int32  compressedSize       length of the zlib stream
int32  size                 length of the uncompressed payload
int32  encryptedSize        compressedSize rounded up to a multiple of 16
byte   data[encryptedSize]  AES-256-ECB(zlib(payload)), last block zero-padded
```

`CSavestateInfo::readHeader` (Linux `0x491c20`) accepts the file only if the magic matches and
26 ≤ version ≤ 30. It is all the save list needs. The `planet`, `time` and `description` fields
exist from version 18 on, so they are always present in readable files.

### Encryption

`ox::core::CAes` ([`CAes.cpp`](../../src/ox/core/CAes.cpp); Linux encrypt `0x5def30`, decrypt
`0x5ded50`) is standard AES-256 in ECB mode, without an IV or chaining. A last block that is not
full is padded with zero bytes, and `decrypt` refuses sizes that are not a multiple of 16. The key
is the same for every save. It is the first 32 values of `ox::algo::CRand(1).nextInt(256)`,
the Park-Miller-style generator `s = (s mod 52774)·40692 − (s div 52774)·3791`, plus
2147483399 if s ≤ 0 ([`CRand.cpp`](../../src/ox/algo/CRand.cpp), Linux `0x5dd0d0`):

```
f490571cb90fe208643cb556f634b50523136dcfad36ed365ae415c6ee6c59b8
```

This was checked by compiling the recovered `CAes`, `CCipherKey` and `CRand` on the host. Their
encryption of 32 test bytes under this key equals OpenSSL's `aes-256-ecb -nopad` output. To read a
save, decrypt `encryptedSize` bytes and inflate the first `compressedSize` of them.

The reader still contains the scrambling of saves older than version 13. Each byte is XORed with
one byte of the 32-bit key `0x4f2c7b19`, highest byte first, and the key rotates right by one bit
after every four bytes. That path is unreachable, because the header check rejects versions below
26.

### Compression

`CFileSystem::zipDeflateData` and `zipInflateData` (Linux `0x52c760` and `0x52c6c0`) wrap zlib
(the program carries version string `1.2.3.3`). Their arguments are
`(target, targetSize, source, sourceSize, &written)`. The output buffer comes first, although
[`IFileSystem.h`](../../src/ox/io/IFileSystem.h) names the first pair "source".

- **Deflate.** `deflateInit_` with level −1 (`Z_DEFAULT_COMPRESSION`), so the output is a zlib
  stream with a 2-byte header and an Adler-32 trailer. Then a single `deflate(Z_FINISH)` and
  `deflateEnd`. `written` is `total_out`.
- **Inflate.** `inflateInit_`, a single `inflate(Z_FINISH)` into a buffer of `size` bytes, and
  `inflateEnd`.
- Both treat a return value of `Z_OK` as success, not only `Z_STREAM_END`. The writer gives the
  compressor an output buffer the size of the uncompressed payload. If the payload did not shrink,
  the save would therefore be silently truncated. Game state compresses well, so this should not
  happen in practice (inferred).

### Payload

The payload is a `CMemWriteFile` built in this order. The "read when" column gives the reader's
version gates that matter for versions 26 to 30. Older gates in the code are always true for
readable files.

| Section | Fields | Read when |
|---|---|---|
| play state | int32 `StartTime`, int32 `RandomValue`, wstring `PlayerName`, wstring `PlayerGroup`, float `ViewPosition.X`, float `ViewPosition.Y`, int32 `GameMode` | always |
| threat level ([`CThreatLevel.cpp`](../../src/HarvestFull/harvest/game/CThreatLevel.cpp)) | int32 game mode (selects the logic class); float `AttackTimer`, float `TimeLeft`, int32 threat level, int32 `Direction`, int32 `AttackCount`, int32 `SectionSeed`; then wave mode only: float `SpawnTimer`, int32 n, n × (int32 alien type, int32 direction), int32 launched waves, int32 reward, int32 wave arriving | always |
| next entity id | int32 | always |
| credits | `CHiddenInt`: int32 key, int32 value XOR key | always |
| outcome | float `GameTime`, int32 `Victory`, int32 `BuildingAttacked` | always |
| mods | uint8 present; if 1, the [Lua section](#lua-section) | version ≥ 29 |
| world ([`CWorld.cpp`](../../src/HarvestFull/harvest/game/CWorld.cpp)) | 3 × 4 floats (left, top, right, bottom) for the target, visible and actual game field; int32 planet; int32 n; n × (int32 doodad type, float x, float y) | always |
| statistics ([`CStatistics.cpp`](../../src/HarvestFull/harvest/game/CStatistics.cpp)) | current level: int32 level, 7 floats, 4 × 14 floats; int32 n; n × (int32 level, float time, 7 floats, 4 × 14 floats); 7 × int32 game stats; float rush-mode damage; int32 n; n × (float time, int32 type, int32 value) | always |
| attack priorities ([`CAlienPriorities.cpp`](../../src/HarvestFull/harvest/settings/CAlienPriorities.cpp)) | 5 weapons × (14 × int32 priority per alien type, int32 range matters, int32 hold fire) | hold fire only in version 30 |
| entities ([`CEntityManager.cpp`](../../src/HarvestFull/harvest/entity/CEntityManager.cpp)) | for each of layers 0, 1, 2 and 3: int32 n, then n entity records | always |

**Masked values.** The credits and the threat-level counters are `CHiddenInt`s in memory: a random
key and the value XOR key, re-keyed now and then. The credits section stores the key and the masked
value as they are, so the credit count is `key ^ value`. The threat level, attack count, section
seed and statistics write their plain values through `getValue()`.

### Entity records

`CEntity::writeEntity` ([`CHarvestEntity.cpp`](../../src/HarvestFull/harvest/entity/CHarvestEntity.cpp))
writes int32 type, int32 id, float x, float y and int32 killed, then the type's `writeEntityData`.
`CEntity::readNextEntity` (Linux `0x432a60`, exact) reads the same five fields and constructs the
entity by type. It restores the id, calls `readEntityData`, and kills the entity again if the flag
was set.

| Type | Class | Data written by |
|---|---|---|
| 0 | spark producer | `CSparkProducerEntity.cpp` |
| 1 | spark mover (energy link) | `CSparkMoverEntity.cpp` |
| 2 | spark | `CSparkEntity.cpp` |
| 3 | construction site | `CConstructionEntity.cpp`: int32 entity type, int32 sparks, int32 sparks needed, string building id |
| 4 | miner | `CMinerEntity.cpp` |
| 5 | minerals | `CMineralsEntity.cpp` |
| 6 | alien | `CAlienEntity.cpp` |
| 7 | laser tower | `CDefenseTowerEntity.cpp` |
| 8, 13, 14 | missile turret variants | `CMissileTurretEntity.cpp` |
| 9 | missile | `CMissileTurretEntity.cpp` |
| 15 | tempest blast | `CMissileTurretEntity.cpp` |
| 16 | creative building | `CCreativeEntity.cpp` |

All of these are under `src/HarvestFull/harvest/entity/`.

Other entity types need care:

- **Particles (type 10)** are not written: `writeEntity` returns at once. They live in layer 4,
  which is never saved, but they would still count towards a saved layer's n if one were in
  layers 0 to 3.
- **Dropships (17), dropship bullets (18) and the shuttle (19)** write only the five header fields.
  `readNextEntity` has no case for them, so they read back as nothing and are dropped.
- **Charge-bomb explosions (12)** are a real bug. `CPerimeterBombExplosion` writes three more
  floats (speed x, speed y, fuse), but `readNextEntity` has no case 12 and skips only the header.
  A save made while an explosion is pending (they live 3 s, in layer 3) is therefore misread from
  that record on.

After loading, `readStateFromFile` runs one entity update of 0.001 s over the view rectangle and
starts the game paused (`GameSpeed = 0`).

### Lua section

Present when mods were running (`CLuaManager::writeLuaStates`,
[`CLuaManager.cpp`](../../src/HarvestFull/harvest/game/CLuaManager.cpp)):

```
int32   n                 running mods
string  folder[n]         each mod's folder name, which identifies it on load
int32   m                 saved values
m × (key attribute, value attribute)
attribute: uint8 isString; then a string if 1, else a float32
```

The values come from the `gameSave` hook. `harvest.lua`'s `hook.call("gameSave", t)` passes every
mod a fresh table to fill. The table is flattened depth-first into dotted keys (`a.b.1`), with
number keys written in decimal. Number values are stored as **float32**, strings as strings.
Booleans, functions and other types are dropped. Keys are always written as strings.

On load (`initLuaBySaveFile`, Linux `0x44fe70`) the reader rescans the mods. For each saved folder
it enables and runs the matching mod, and silently skips folders it cannot find. It then rebuilds
the nested table (a path component that parses as a non-zero integer, or is `"0"`, becomes a number
key) and passes it to `hook.call("gameLoad", t)`.

If n ≤ 0, the reader returns at once (`jle` at Linux `0x44fea0`) without reading m, and the game
continues without Lua. The writer always writes m, though, so such a file leaves 4 unread bytes in
front of the world section. n = 0 happens in a creative game that has mods available but none
enabled: `initializeNewGame` creates the Lua manager whenever any mod exists, the shipped mods start
unchecked, and the manager writes this section even when it runs nothing. On paper, such a save
loads shifted by 4 bytes. This follows from the code and was not tried in the game. A port that
wants these saves to load should skip m when n is 0.

## Profile and settings files

Both are `ox::game::CConfiguration` files: `block { attribute = value; }` text, addressed as
`block:attribute` ([`CConfiguration.h`](../../src/ox/game/CConfiguration.h)). `write` produces
UTF-16 with a `0xFEFF` byte order mark. `read` also accepts 8-bit text in the current locale.

**Profile** ([`CHarvestProfile.cpp`](../../src/HarvestFull/harvest/settings/CHarvestProfile.cpp)):

| Attribute | Value |
|---|---|
| `profile:name`, `profile:group` | player name and group, cut to 16 characters on load |
| `prio:lasers`, `prio:linkedLasers`, `prio:missiles`, `prio:eagles`, `prio:tempest` | comma-separated priority per alien type (default `2,2,2,2,2,2,2,2,2`) |
| `prio:<weapon>Dist` | 1 if range matters (default 1 for eagles only) |
| `localscores:levels<p>`, `localscores:minerals<p>`, `localscores:times<p>` | for planet p = 0 to 2, five comma-separated scores, one per game mode |
| `localscores:achievements` | base64url of int32 1, then 33 bytes, one per achievement (25 main, 8 mini), of flags: 1 complete, 2/4/8 done on planet 1/2/3. A byte with any of the high four bits set reads as 0 |
| `settings:keyboard` | base64url of int32 1, then 255 bytes: the command for each key code |

base64url here is the URL-safe alphabet `A–Z a–z 0–9 - _` with `=` padding
([`CBase64url.cpp`](../../src/ox/algo/CBase64url.cpp)).

**`harvest.cfg`** holds the attributes in this table. `getAttributeAsInt` and `getAttributeAsFloat`
parse the stored text.

| Attribute | Meaning |
|---|---|
| `settings:firstrun` | 1 until the first-run window has been confirmed |
| `settings:language` | the language file's path |
| `settings:resolutionh`, `settings:resolutionv` | window size |
| `settings:fullscreen` | 0 or 1 |
| `settings:driver` | `OGL` or `DX9` |
| `settings:shaderlevel` | 0 to 2, default 2, see [menu-scene.md](menu-scene.md#shader-levels) |
| `settings:volumesfx`, `settings:volumemusic` | 0 to 5, default 3 (see [audio.md](audio.md)) |
| `settings:particles` | 0 to 2, default 2 |
| `settings:scrollspeed` | float, default 1 |
| `user:recentProfile` | the last profile's file name |
| `settings:system` | the license key, stored with `setAttributeAsBase64` (base64url of its UTF-16 units and terminator) |
