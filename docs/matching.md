# Matching

`hv match` compiles each recovered source file listed in `config/<build>/units.toml` in the pinned
toolchain container and compares the object with the target image, section by section.

## Names: `symbols.tsv`

`hv port-symbols` fills `config/1.18-linux-amd64/symbols.tsv` from the target itself:

- **RTTI.** The executable holds libstdc++'s three `__cxxabiv1::*_type_info` vtables as copy-relocated
  data, and every typeinfo object starts with a pointer into one of them. Scanning for those three
  values finds all 350 typeinfos (`_ZTI`), each pointing at its name string (`_ZTS`). A word holding a
  typeinfo address after a zero offset-to-top is the class's primary vtable (`_ZTV`).
- **Virtual functions.** Each Linux vtable is walked beside the Mac vtable of the same class, word by
  word, while the two agree: typeinfo words must point at the same class, and a function slot is named
  only if the Mac word is a Mac text symbol and the Linux word points into `.text`. An address that
  gets two names, or a name that gets two addresses, is dropped. Every one of the 2,238 named
  functions starts an FDE, which gives its Linux size.
- Thunks are skipped: their mangled names encode the i386 `this` adjustment.

Rows with other evidence (for example `manual:`) are kept when the generator reruns.

Mac virtual-slot names need instruction-level validation when the layouts differ. Linux's
`CFileSystem` inserts a path-cache-clearing virtual at slot 12, shifting subsequent Mac names by
one. Manual rows correct those shifted names. The inserted method's original name is unavailable;
the recovered interface uses the descriptive name `clearCachedFilePaths`. `existFile` is Linux
slot 17 (`vptr + 0x78`), not Mac slot 16.

## Placement

Every allocated object section is placed at a target address:

- by the symbols it defines that `symbols.tsv` knows. The earliest one anchors the section, so a
  single known function places the whole `.text`; every other known symbol must land at its known
  address, or the section is inexact and the report lists it as misplaced;
- or explicitly in `units.toml`, for sections with no known symbol (such as `.bss`).

Inline functions, vtables and RTTI that GCC emits as COMDAT sections are placed at the kept copy.
That copy may come from another object; its bytes are still compared, which checks the header code.
Merged string and constant sections are not placed, because the linker merges and deduplicates them;
each reference into them is checked by comparing the referenced string or constant in the target.
`.eh_frame`, `.ctors` and similar sections are not compared yet. A section that cannot be placed
makes the unit inexact.

An allocated `.bss` section must fit completely inside the target's NOBITS section, and every
known symbol must agree with its placement.

## Comparison

Relocations are resolved against placed sections, `symbols.tsv`, the target's PLT (decoded through
its GOT relocations), and copy-relocated library data, then written into the object bytes. Supported
types are `R_X86_64_64`, `PC32`, `PLT32`, `32` and `32S`; anything else, an unknown symbol, or an
overflow makes the section inexact. Nothing is masked.

A placed section matches when its relocated bytes equal the target's over its whole size. In
executable sections, each function must also start an FDE of exactly its size in the target, which
pins the extent of the last function too. A unit is exact when every placed section matches and none
is left unplaced. Reports go to `build/match/<build>/<unit>.json`: placements, bad references,
differing bytes, per-function results, and compilation provenance (container image ID, compiler
version, package manifest hash, command, and hashes of every repository input from GCC's dependency
file).

## objdiff

`hv match` also writes, for each unit, a target object cut out of the executable
(`build/objdiff/<build>/target/`), a copy of our object (`.../base/`), and `objdiff.json` at the
repository root (gitignored), so objdiff's GUI can open the repository as a project. `just objdiff-cli`
downloads the pinned objdiff-cli 3.8.1 (checksummed) into `build/tools/`, and
`hv diff <unit> <symbol>` prints the differing instructions of one function, target on the left.

The target object (`tools/hv/delink.py`) holds the target's bytes laid out like our object: the same
sections, each function at our offset. Its relocations come from the target code: capstone decodes
every instruction, and each call or jump leaving the function, rip-relative operand and absolute
address immediate becomes a relocation against the symbol the target references (known symbols, the
PLT, copied library data, our placed sections by offset; `sub_`/`lbl_` otherwise). Calls to a local
function in the same section are resolved in place, as the assembler did for ours. Referenced strings
that our object also has go into a target copy of our merged string section at the same offsets,
holding the target's bytes. Nothing is copied from our relocations, so a wrong target stays visible.
Indexed absolute operands also get relocations. Binary merge elements are checked by the memory
operand's access width and content, separately from strings; non-allocated metadata such as
`.comment` cannot supply literals.
With these objects objdiff scores every function of the exact units at 100%, matching `hv match`.

## Tests

- `uv run pytest`: synthetic ELF fixtures (call destinations and addends, unknown symbols,
  unsupported types, overflow, overlaps, changed constants, FDE extents, BSS bounds and placement
  conflicts, mutable static substitution, indexed addresses and binary literals); with
  the originals present, RTTI and vtable porting.
- `HARVEST_TEST_TOOLCHAIN=1 uv run pytest`: compiles `ox/io/CMemReadFile.cpp`, requires an exact
  match, and requires three mutations to fail: a changed constant, `memcpy` changed to `memmove`, and
  two virtual declarations swapped (same function bodies, different vtable).

## Recovered so far

| Unit | Functions | Notes |
| --- | ---: | --- |
| `daisy/video/Null/CColorConverter.cpp` | 16/16 | exact; palette/packed image conversion, resizing and encoded 32-bit channel-layout conversion |
| `ox/algo/CRegulator.cpp` | 34/34 | exact; scalar and three-axis PID-style regulators, anti-windup and speed regulation |
| `ox/game/CGameState.cpp` | 10/10 | exact; state initialization, cached device subsystems and borrowed error messages |
| `daisy/video/Software/CZBuffer.cpp` | 16/16 | exact; signed 16-bit software depth buffer, resizing and reference-counted factory |
| `ox/io/CMemReadFile.cpp` | 19/19 | Irrlicht 0.7 `IUnknown` → `IReadFile` → `CMemReadFile` |
| `ox/io/CMemWriteFile.cpp` | 18/18 | `IWriteFile` from Irrlicht 0.7; the class itself is Oxeye's |
| `ox/net/CHTTPConnectionHandler.cpp` | 18/19 | `OnEvent` differs only in one register choice; function order differs |
| `ox/net/CVariablePacket.cpp` | 32/32 | packet parser and builder; function order differs |
| `ox/io/CHelpIO.cpp` | 19/19 | Complete unit: numeric and string I/O, free-filename selection, static initializer; all compared sections match |
| `ox/algo/CRand.cpp`, `CSimplePress.cpp`, `CTimeCounter.cpp` | 35/35 | exact |
| `ox/core/CBasic.cpp`, `CCipherKey.cpp`, `CCriticalSection.cpp`, `CHiddenFloat.cpp`, `CHiddenInt.cpp`, `CThread.cpp` | 61/61 | exact |
| `daisy/video/Null/CFPSCounter.cpp` | 5/5 | exact; Irrlicht 0.7 FPS calculation, both constructors and the iostream initializer |
| `HarvestFull/harvest/game/CThreatLevel.cpp` | 76/81 | game modes and waves; five functions differ only in register allocation (and one switch layout) |
| `ox/entity/COxEntity.cpp` | 25/25 | exact; keeps the 2d constructors' `Position.Y` typo |
| `HarvestFull/harvest/entity/CBuildingEntity.cpp` | 31/31 | exact; the building's Lua view (Lunar method table) |
| `HarvestFull/harvest/entity/CSparkProducerEntity.cpp` | 28/28 | exact; the solar collector |
| `HarvestFull/harvest/entity/CSparkMoverEntity.cpp` | 34/40 | energy links: waypoints, heat, overcharge; `onSpark` block layout and a few register choices differ |
| `HarvestFull/harvest/entity/CMinerEntity.cpp` | 31/35 | the mineral harvester and its energy beam; `getInfoString` block order, one `updateLogic` tail and a loop-compare operand order in the constructor differ |
| `HarvestFull/harvest/entity/CConstructionEntity.cpp` | 30/30 | exact; construction sites and calling spark movers |
| `HarvestFull/harvest/entity/CMineralsEntity.cpp` | 33/33 | exact; mineral deposits and the level scatter |
| `HarvestFull/harvest/entity/CSparkEntity.cpp` | 27/27 | exact; a spark homing on its target building through an `SEntityReference` |
| `HarvestFull/harvest/entity/CHarvestEntity.cpp` | 69/70 | `CEntity`, particles, special effects, spark search; `selectSparkTarget` differs in register allocation |
| `HarvestFull/harvest/entity/CPerimeterBomb.cpp` | 24/25 | three-second fuse, radial alien damage, bomb knockback and kill events; `updateLogic` differs only in three loop-compare operand orders |
| `HarvestFull/harvest/settings/CAlienPriorities.cpp` | 11/11 | weapon targeting weights, range preference, hold-fire and serialization; destructor section placement differs |
| `HarvestFull/harvest/entity/CDefenseTowerEntity.cpp` | 46/50 | the laser defense tower: linking, targeting, beam update and sprite setup. The constructors differ only in which `CString` temporaries GCC inlines (19/107 against our 20/106), and `updateLogic`/`updateTowerLinks` only in block layout and register allocation; no remaining semantic differences |
| `HarvestFull/harvest/entity/CMissileTurretEntity.cpp` | 70/77 | missile acceleration, retargeting toggle, tempest lightning geometry and damage, projectile construction, animations, culling and serialization; turret constructors, target selection and missile/launch combat loops remain inexact |
| `HarvestFull/harvest/entity/CAlienEntity.cpp` | 59/67 | alien layout, constructors, save/load, Lua control, damage, all movement AI, target search, rendering, update logic and sprite sets, checked against the target; inexact: `updateSprite` registers, miner movement layout, two iterator compare operand orders in `locateTargetBuilding`, `render`/`updateLogic` scheduling, and three large sprite setups whose `CString` inlining choice follows GCC's unit-wide inline order |
| `HarvestFull/harvest/entity/CDropshipEntity.cpp` | 53/55 | the dropship and its bullet: flight state machine, missile salvos, landing and takeoff, engine sound. `updateLogic` has the target's instructions in a different block layout, and `loadSprite` (98%) inlines one `CString` constructor fewer because the original unit was slightly larger (the inline-unit-growth budget); no remaining semantic differences |
| `HarvestFull/harvest/entity/CShuttleEntity.cpp` | 42/44 | race placement and sorting, steering AI, checkpoints, rendering and state accessors; recovery movement and AI training remain inexact (the constructors matched once their C1/C2 labels were swapped: only shuttle-race `new` calls 0x43e670) |
| `HarvestFull/harvest/entity/CCreativeEntity.cpp` | 52/55 | creative buildings, Lua energy/color/progress/sprite controls, private-table serialization, constructors and rendering; the metadata loader and two empty display strings differ only in register or loop-compare choices |
| `HarvestFull/harvest/entity/CEntityManager.cpp` | 38/43 | building lists and bounds, spatial grid maintenance, cached targeting searches, entity construction, save/load, and energy-beam rendering; placement, grid range search, and two predicates remain inexact; the constructor flips one compare with the declaration count of included headers |
| `HarvestFull/harvest/gfx/CScatterShader.cpp` | 12/17 | planet scattering shader: construction, Cg material setup and per-frame shader constants with inlined `CMatrix4` math; the base-object constructor differs only in store scheduling, and the four virtual thunks have no FDE |
| `ox/entity/COxEntityManager.cpp` | 20/21 | entity ownership, deferred spawning and removal, reference refresh, first/best/all targeting searches, and render ordering with native sort helpers; the update loop remains inexact |
| `HarvestFull/harvest/game/CWorld.cpp` | 30/38 | complete world source: scenery collision grid, wind forces, sprite loading, scenario startup, edge shading, camera bounds and serialization; placement, collision avoidance, expansion, background rendering and the main update are reconstructed but inexact |
| `HarvestFull/harvest/game/CStatistics.cpp` | 36/41 | wave summaries, protected score totals, event logs, versioned serialization and highscore accessors; score encoding, parameterized score construction and two statistic update routines are reconstructed but inexact |
| `HarvestFull/harvest/game/CScenario.cpp` | 62/64 | complete campaign source: the 200-call starting map, lifecycle, dialogue, dropship landing and departure, mineral-triggered reinforcements, completion conditions and event callbacks; the scheduler and opening dialogue/credits sequence remain inexact |
| `HarvestFull/harvest/game/CLuaManager.cpp` | 167/204 | the `harvest` Lua library (61 bindings, named from the Linux registration table), hooks, mods, Lua save values and the Lunar templates; remaining misses are mostly register choices in inlined `CString` copies and the unit's inlining budget (`subString`, vector insertions) |
| `HarvestFull/harvest/settings/CSystemConfig.cpp` | 42/48 | settings file accessors, language loading, the out-of-line `getCurrentLanguage` (as on Mac), `applySettings`, `getLocalizedText` and `replaceAll<wchar_t>`; the constructors, `getAllLanguages` and `replaceAll` differ only in compare operand order in inlined `CString` loops, and `setRecentProfile`/`addLicenseKeySetting` only in their strlen-loop registers (both follow SSA numbering) |
| `HarvestFull/harvest/settings/CSavestateInfo.cpp` | 3/3 | exact; save game header read/write |
| `HarvestFull/harvest/states/CIntroState.cpp` | 12/13 | the splash state: step-wise loading (all 37 steps are explicit cases, so idle steps and the done check get separate jump-table targets), fades, layer slide and rendering; `OnEvent` has the same blocks as the target but lays out the mouse branch's inlined skip logic differently |
| `HarvestFull/harvest/states/CLoadingScreen.cpp` | 8/8 | the spinning logo and version line drawn while states load |
| `HarvestFull/harvest/gui/CIngameMenuScreen.cpp` | 9/10 | the in-game menu: button frame, abandon-game confirmations and the shared `sendCustomEvent` helper; `OnEvent` matches except that the identical save and awards blocks are laid out in the opposite order |
| `HarvestFull/harvest/gui/CMenuInfoDialog.cpp` | 12/13 | the tutorial briefing dialog: voice lines, typing dots, the start/close buttons and `refillMainView` (portrait texts, progressive reveal timing); `OnEvent` places the tutorial-start block differently |
| `HarvestFull/main.cpp` | 2/2 | the program entry running the `CHarvestFullMain` loop |
| `HarvestFull/harvest/CHarvestFullMain.cpp` | 19/19 | game-wide globals (`g_loadGameFilename` starts as `""`), device and settings start-up (`init`; the Linux build has no Steam), the state factory, the shared input receiver (button sounds tested before the checkbox, Ctrl+T screenshots, fullscreen toggle) and teardown; its `vector<CString<wchar_t>>` insert copy has been sensitive to the declaration count of shared headers |
| `HarvestFull/harvest/gui/CSettingsScreen.cpp` | 20/22 | the settings window: general tab (audio sliders, 32-bit resolution list of at least 800x600, fullscreen, particle and scroll-speed sliders, language list) and key-binding tab, control helpers, screen-mode change and key rebinding; `.rodata` holds the `KEY_NAMES` and `KeyCommandNames` tables. `OnEvent` (96.8%) has two small blocks and the landing pads in swapped order and its Escape path uses other registers; the `~TArray<SLanguageFile>` COMDAT copy differs only by trailing padding |
| `HarvestFull/harvest/gui/CGuiEffects.cpp` | 3/3 | the screen-darkening overlay with holes around entities of one type (`renderRecangleOverlay`) and its rectangle splitter |
| `HarvestFull/harvest/gui/CGuiInfoLines.cpp` | 9/9 | the in-game message list: lifetime, `update` and both `addInfoLine` overloads |
| `HarvestFull/harvest/gui/CAchievementsScreen.cpp` | 19/20 | the award board: layout, rating title, award tooltips, the Medusa dialogs and the wobbling wicked-awesome award, with the award position and Medusa text tables; the only miss is this unit's copy of the `CString<char>::operator=` COMDAT (one register), whose kept copy belongs to another object |
| `HarvestFull/harvest/gui/CPriorityScreen.cpp` | 17/20 | the alien priority screen: priority boxes dragged between rows, sprites, target-closest/hold-fire boxes and saving to the profile; `OnEvent` (block placement, store order and two swapped compares), `updatePriorityBoxPositions` (two `lea`/`add` operand orders) and this unit's `CString<wchar_t>::operator=` COMDAT copy remain inexact |
| `HarvestFull/harvest/states/CShuttleRaceState.cpp` | 23/33 | the shuttle race minigame state (bases `CGameState`, `IScenario`, `IParticleEngineCallback`): OnEvent, particle callbacks, firstInit, lifetime and the `LAP_CHECKPOINTS` initializer; `secondInit` lays out its case blocks differently, `updateState` inlines string helpers the target calls, `render` (6.5 KB) is about 78% similar, and seven non-virtual thunks have no FDE |
| `HarvestFull/harvest/entity/CBuildableItems.cpp` | 31/33 | the buildable buildings list: standard and creative loaders, `addBuilding` (4.8 KB), special upgrades, lookups, button rendering and vector helpers; `loadCreativeBuildingList` (5.8 KB, about 97%, register allocation and compare order) and `changeConstructionSelection` (loop shape) remain |
| `HarvestFull/harvest/settings/CHarvestProfile.cpp` | 43/47 | the profile settings file: name and group, local scores, achievements, key mapping and weapon priorities, plus the `splitString<wchar_t>` copy; `getAchievementSpriteName` (register and stack-slot choices), `getFilename` and `parseLocalAchievementsString` (registers in inlined `CString` copies) and the shared `vector<CString<wchar_t>>` insert helper remain |
| `HarvestFull/harvest/settings/CProfileManager.cpp` | 15/17 | the profile list: open, create and delete, and `gp_profileManager`; `createProfile` and `createProfileList` differ only by registers in inlined `CString` loops |
| `HarvestFull/harvest/gui/CProfileScreen.cpp` | 16/18 | the profile list and create/edit windows, name validation and trimming; `saveProfile` lays out two error branches in swapped order and `OnEvent` its button cases |
| `HarvestFull/harvest/gui/CStatisticsScreen.cpp` | 21/24 | the post-game statistics screen: wave graphs with fade-in, event log and totals with paging, local bests, highscore upload over HTTP and the window/graph draw handlers; `createGraphs` (load/store scheduling), `OnEvent` (block layout and one call the target inlines) and `renderGraph` (bar width reloaded every iteration) remain |
| `HarvestFull/harvest/gui/CStoryScreen.cpp` | 15/16 | the campaign dialogue letterbox: two bordered lines with portrait, name and progressively revealed text, fades, voice playback and the skip key; `OnEvent` (97%) tests the window id before the event type |
| `HarvestFull/harvest/gui/CHighscoreScreen.cpp` | 26/33 | the online highscore screen: summary pages with promote buttons, highscore table parsing, HTTP requests, Base64/UTF-8 name coding and filter popups, plus the `splitString<char>` and `CString<wchar_t>` assignment COMDATs; `OnEvent` (98%, cold-block `imul` and tail duplication), `parseSummaryPage` and `update` (one swapped compare each in an inlined `CString` copy), `setVisible`/`createStatusString` (registers) and `parseHighscoreString` remain. `parseHighscoreString` matches at 99.7% as an `if (columns.size() == 7)` block, but that form shifts the unit's inline budget and breaks `loadHighscores`, so the source keeps the `continue` form |
| `HarvestFull/harvest/gui/CSaveGameScreen.cpp` | 31/36 | the save/load screen: the profile's save games sorted with `std::sort`, delete/overwrite message boxes, description box and mode/planet icons (`SSaveListItem`, `SSavestateHeader`); all twelve methods and most sort helpers are exact. `__pop_heap`, `__unguarded_linear_insert` and `__insertion_sort` differ by registers and compare order, `__unguarded_partition` inlines one more item copy than the target (the original hit the unit's inline budget earlier), and this unit's `CString<char>::operator=` copy differs by a register. The item's empty destructor and hand-written assignments, and defining `createSaveGameBox` before `OnEvent`, were chosen for the inlining order they produce |
| `HarvestFull/harvest/states/CMainMenuState.cpp` | 27/31 | the main menu state: planet scene, camera fly-over, console buttons, planet and game-mode selection, mod and demo dialogs; `secondInit` has every block of the target but a different case order from bb-reorder, `OnEvent` inlines a `CString<wchar_t>()` the target calls (shifting its frame), `enterGameModeSelectMode` differs by registers and the `CString<char>::operator+=` COMDAT by one swapped compare |
| `HarvestFull/harvest/states/CPlayState.cpp` | 70/98 | the in-game state: GUI setup, input, rendering, minimap, save/load, wave and creative lists and the game loop. `OnEvent`, `updateState`, `renderMinimap` and `newSelectedEntity` were checked against the target and do what it does, but differ in block layout, register allocation, stack slots and float scheduling; `readStateFromFile` is exact. `buyBuildingAtPlacementPos`, `updatePlacementPosition`, `secondInit`, `initializeNewGame`, `writeStateToFile` and `setWaveListToggle` differ only in operand order, loop rotation or registers; three non-virtual thunks have no FDE, and three `CConditionalHarvestEvent` copies come from `CScenario.h` declaring `runEvent` out of line but defining it `inline`, which makes it the key function in every includer. The `.rodata` pin is where the unit's jump tables start |
| `daisy/video/Null/CParticlePackage.cpp` | 28/31 | the particle package reader (format versions 0-8: type info, waveform functions, '/'-split name lists), types sorted with `std::sort` and found by `ox::algo::binarySearchIf`, importance filter, state creation and removal; `__insertion_sort`/`__introsort_loop` (swapped compares in the inlined `CString::operator<`) and this unit's `vector<CString<char>>` insert copy remain. Defining `load` after its helpers made four functions exact at once |
| `daisy/video/Null/CParticleState.cpp` | 29/30 | particle behaviour: random animation, start speed and direction, lifetime, colour and scale fades, eight waveform types, bounces, wind, start sound, timed pulses, on-die particles chosen by '@' marker groups, 2D/shadow/3D rendering; only `update` (94%, registers, one compare and two block placements) remains. Keeps the original's reversed direction/elevation ranges (`min - max`) |
| `daisy/audio/CAudioDriver.cpp` | 67/70 | the sound logic over the OpenAL backend: volume settings as gains, voice ducking, tracked-sound handles, string-keyed sound and music maps, and the device hooks the backend overrides; `findOrLoadSound`, `loopSound` and `playMusic` differ only in register allocation and where string-destructor cleanups sit |
| `daisy/gui/CGUIFont.cpp` | 20/22 | Irrlicht's bitmap font with Oxeye changes: character pixels keep their texture colours, right and bottom alignment, `std::vector` storage; both constructor copies are byte-identical but call the local `reserve` clone `T.189`, whose address depends on the unit's .text order, which is not reproduced |
| `daisy/gui/CUnicodeFont.cpp` | 17/17 | exact; the sprite-package font: characters are animations named by decimal code, kept in 1999 hash buckets and loaded on first use, with `#0`-`#9` and `#o` colour codes |
| `daisy/video/Null/CSpriteAnimationState.cpp` | 32/32 | exact; a running sprite animation: frame advance with per-frame jumps and millisecond durations (negative holds for 48 h), save/load, and per-frame forwarding to the shared images |
| `daisy/video/Null/CSpriteAnimationImageBundle.cpp` | 26/27 | a bundle drawn as a grid of sprite images (only the plain draw is implemented); `getSize` differs in register allocation |
| `daisy/video/Null/CSpriteAnimationImage.cpp` | 22/26 | one sprite image drawn plain, scaled, mirrored, rotated, free-shape, with corner or per-vertex colours, or as a 3D quad; all values match the target. `drawRotated` and `drawMultipleColors` differ in scheduling, `drawMirrored` in stack slot order and an alias reload of `position`, and `draw3d` (with the target's per-vertex `mirrored ?:` texture coordinates) in material store order and registers |
| `daisy/video/Null/CSpritePackage.cpp` | 67/71 | the `.dat` sprite package format (header, lazily decoded texture planes, sprites, bundles and animations; layout in `CSpritePackage.h`), id-sorted image and state lists with binary and linear search; definition order follows the Linux layout. `load` differs in registers, loop rotation and one `~SAnimationData` inlining choice, `removeAnimationState` by one swapped compare, and two `vector<CString<char>>` copies owned by CLuaManager. Sprite and bundle records must stay plain old data (the shared header is a member, not a base) or libstdc++ stops using `memmove` and every sort and vector template changes |
| `ox/game/CConfigBlock.cpp` | 22/23 | a named block of (name, value) `CString` attributes: lookup, typed get/set overloads and `printBlock`; only `getAttribute(CString)` has one swapped compare in the inlined `CString::operator=` self-check |
| `ox/game/CConfiguration.cpp` | 34/39 | the configuration file format used for settings, profiles, languages and mods: `name { attr = value; }` blocks with `#` comments, backslash-escaped `;`, 0xfeff-marked 16-bit files (else locale text), base64 attributes. `getBlock(const wchar_t*)` is defined before `blockExists(const wchar_t*)`, which gives the build's registers. Four functions differ only in string-length-loop registers or one compare order; `parseString` has the target's control flow, calls and accesses but not its block layout |
| `ox/event/IEventReceiver.cpp` | 17/17 | event receivers and the global subscriber list: subscribe, unsubscribe and delayed events buffered twice under a critical section, with the two `vector::_M_insert_aux` copies |
| `ox/game/CGameMain.cpp` | 15/15 | the main loop: creates the subscriber list, switches states, steps frames by elapsed time, sleeps while inactive, runs delayed events, and reports missing data files in a message box |
| `ox/core/AlwaysAppendToFile.cpp` | 2/2 | `vsprintf` into a 1 KB buffer appended to a log file (the unit name is ours) |
| `ox/core/CAes.cpp` | 29/30 | AES-256 used for save scrambling: key expansion, GF(2^8) tables, block encrypt/decrypt with a zero-padded last block, and the `ICipher` defaults; `expandKey` differs by one swapped loop compare |
| `ox/algo/CBase64url.cpp` | 4/5 | base64url encode with `=` padding, `encode2` (standard alphabet with `+`, `/` and `=` percent-encoded) and decode through a `-`..`z` table; `encode2` differs only by two swapped compares in inlined `CString` appends |
| `ox/gui/IGUIElement.cpp` | 10/14 | `remove` is the key function, so the unit holds the element vtable, destructors and inline method copies; `updateAbsolutePosition` swaps a compare, `removeChild` inlines `drop` and `erase` where the target calls them, and the two `_ZThn24_` thunks have no FDE |
| `ox/game/CTextLocalization.cpp` | 3/4 | `getText` looks up the text attribute and turns `\\n` into newlines; the shared `replaceAll<wchar_t>` copy is inexact |
| `ox/core/CMath.cpp` | 7/9 | distances and angles, `pointInFrontOfLine`, segment intersection and Catmull-Rom `hermiteInterpolation`; `lineIntersects` differs in layout and tail merging, `hermiteInterpolation` in commutative operand order and scheduling |
| `ox/core/CStringFunctions.cpp` | 1/2 | `millisecondsToWide` formats `[h:]mm:ss.hh`; one swapped compare in an inlined `CString` constructor loop |

Counts include inline methods and base-class destructors emitted as COMDAT copies. The HTTP handler
brought in `CString` (Irrlicht's `string` plus Oxeye's methods), `TArray`, `CStringFunctions`,
`SEvent`/`IEventReceiver` (network event only), `IOxDevice`, `INetworkDevice`/`SServerInfo`, and
declarations of `CCriticalSection` and `CThread`.

The complete `CColorConverter` unit matches its 2,742-byte `.text` at `0x5d8300`
and one-byte `.bss` at `0x86c1f8`. Its 15 conversion routines and iostream static
initializer contribute 2,588 unique function bytes. The namespace and signatures
come from the Mac symbols; Linux FDE extents and exact loop bodies pin their
addresses. Shared RGB packing helpers preserve Harvest's forced `0x8000` alpha
bit, unlike the unmodified Irrlicht 0.7 helper.

The two Harvest-specific 32-bit routines use a color-format descriptor containing
four channel shifts, not an ordinary sequential format ID. The scalar helper
returns the input unchanged for identical descriptors; the buffer helper also
supports in-place conversion. Known row/pitch quirks are kept: non-flipped 8-bit
palette reads add an extra pitch after the first row; 32-to-16 flip output starts
at `(width + pitch) * height` shorts; the 32-to-32 flip ignores pitch. The resize
routine expands five-bit components without filling their low bits and expands
the one-bit alpha to bit 31 rather than to eight set alpha bits.

`HARVEST_TEST_TOOLCHAIN=1 uv run pytest -q tests/test_color_converter.py` compiles
and executes a checked-in native smoke covering all conversion routines. It
checks odd-width packed palettes, unsigned 8-bit palette indices, monochrome
bits, row orientation and pitch quirks, RGB shuffling, resizing, zero dimensions,
and every pair of the 24 four-channel layouts (576 pairs) across six pixel
patterns. Buffer and scalar results are checked against an independent shift
model, with in-place and destination-guard checks. Missing opaque alpha and a
wrong selected channel are required to fail. The matched object was also linked
and exercised directly. The complete game was not executed.

The complete `CRegulator` unit matches its 2,660-byte `.text` at `0x5dd200`,
`.bss` at `0x86c210`, and both classes' vtables and RTTI. The scalar controller
is 56 bytes and the three-axis wrapper is 176 bytes on Linux amd64. All 34
functions match, including constructors, destructor variants, forwarding methods,
the scalar update, the speed helper and the static initializer.

The native integral accumulates error without multiplying by time. Anti-windup
clamps it symmetrically; `restart` seeds it from the configured start value only
when anti-windup is enabled. The constructor does not use that seed. The
derivative uses previous error minus current error and is evaluated only after
accumulated elapsed time exceeds the double constant `0.01`, then resets its
elapsed counter. These details are preserved rather than replaced with a standard
PID formula.

The speed helper ignores its first float argument. It accelerates toward the
normalized desired velocity with the native constant `20.0f`, clamps component
overshoot according to the desired component's sign, then advances position.
A zero desired vector leaves both speed and position unchanged; a zero component
does not clamp an existing speed component. Direct component-wise zero tests
reproduce the native block layout; the overloaded vector comparison does not.

`HARVEST_TEST_TOOLCHAIN=1 uv run pytest -q tests/test_regulator.py` runs the checked-in
native smoke against a canonical compiled object in the pinned container. It
checks scalar P/I/D behavior, derivative timing, positive/negative anti-windup,
restart and setters, zero/negative timesteps, 64 three-axis forwarding steps,
speed acceleration and signed overshoot, zero-vector behavior, the unused
argument, layouts and destruction. Mutations changing acceleration to `10.0f`
or time-weighting the integral are required to fail. The matched object was also
linked and exercised directly. The complete game was not executed.

The complete `CGameState` unit matches `.text` at `0x5ea6f0` (457 bytes), `.bss`
at `0x874270`, and its vtable and RTTI. It inherits `IEventReceiver` and has a
72-byte amd64 layout. Initialization fetches video, GUI, scene, audio and joystick
subsystems in that order before checking the three required video/GUI/scene
pointers. Audio and joystick are optional. Both initialization helpers return 1
for failure and 0 for success; custom error messages are borrowed, not copied.

Native quirks are preserved: the constructor does not initialize audio or joystick
fields, a null-device retry leaves cached subsystem pointers unchanged, and a
successful retry does not clear a previous error message. A behavior smoke linked
the matched object in the pinned GCC 4.4.3 container with a mock device and a stub
for the external `IEventReceiver` destructor. It checked constructor writes,
all 32 combinations of available subsystems, getter order, null-device failures,
failure/success retries, error-pointer aliasing and virtual destruction. The
external event-unsubscription implementation and complete game were not executed.


The complete `CZBuffer` unit matches `.text` at `0x4f8010`, `.bss` at `0x86c078`,
and its exception table, vtables and RTTI. The Irrlicht 0.7 implementation retains
signed 16-bit depth values and the native 56-byte amd64 layout. Resizing to the same
dimensions preserves the allocation and its contents; a changed size reallocates
without initializing the depth values. `clear` zeros the entire buffer.

A behavior smoke linked the matched object in the pinned GCC 4.4.3 container and
checked the class size, factory and virtual dispatch, clearing all elements,
height-only and width-only resize, unchanged-size content preservation, zero-size
construction and clearing, and reference-counted destruction. The complete game
was not executed.

The complete `CHelpIO` unit matches `.text` at `0x5eaf90`, `.bss` at `0x87427c`, and its
103-byte `.gcc_except_table` at `0x666e8b`. Exception-table placement is independently pinned by
the `readWideString` FDE's LSDA pointer and the unique occurrence of the complete table's bytes.
Numeric reads initialize their values to zero and ignore the read result. Narrow and wide string
readers append 255-character chunks; wide-string writes truncate each `wchar_t` to 16 bits rather
than encode supplementary Unicode characters. A nonpositive length prefix leaves the destination
string unchanged. Filename selection starts at `00` and skips existing names.

The behavior smoke linked the matched helper object with the recovered memory-file classes in the
pinned Linux GCC 4.4.3 container, without the build-only timing library. It checked numeric wire
bytes, EOF and short reads, string lengths around both 255- and 510-character chunk boundaries,
terminated and unterminated strings, 16-bit wide-character truncation, counted strings with
embedded NULs, and zero/negative/maximum decimal appends. A disk-backed filename smoke selected
`00` in an empty directory, then `11` after creating files `00` through `10`. The complete game
was not executed.

The complete `CFPSCounter` unit matches its 198-byte `.text` at `0x599c90` and one-byte
`.bss` at `0x86c160`. The Mac unit identifies the methods; the Linux FDEs pin their extents,
and the unchanged Irrlicht 0.7 calculation identifies the 65-byte `registerFrame` body at
`0x599ce0`. Its 1000.0f constant and every relocation are checked without masking.
The counter starts with 100 counted frames, increments before testing elapsed time, updates
only after strictly more than 2000 milliseconds, truncates the floating-point FPS result to
an integer, and resets the sample. Unsigned subtraction preserves clock rollover behavior.
A smoke executable linked against the matched object in the pinned GCC 4.4.3 container
checked initial state, the exact threshold, accumulated frames, truncation, reset, and rollover.
The complete game was not executed.

Findings:

- Linux function order within `.text` follows GCC 4.4's `cgraph_expand_all_functions`: the reverse
  of `cgraph_postorder`, which walks the node list newest first and emits callers before callees.
  Inline copies are prepended to that list as the IPA inliner creates them, and each copy has the
  function it was inlined into as its only caller, so a function's position follows the time of the
  *last* inlining decision into it. That order comes from the inliner's badness heap, which depends
  on the estimated sizes of the inline helpers (`CString`, `SServerInfo`, `wideToAnsi`). Two sources
  that compile to the same bytes can still order differently, so the order carries information about
  the exact form of shared inline code. CHTTPConnectionHandler's order is still open: Linux has
  `C2 C1 joinThread <clone> doGet`, ours `<clone> C1 C2 doGet joinThread` (definition order already
  follows Mac, which keeps source order). Useful tools: `-fdump-ipa-cgraph -fdump-ipa-inline`.
- GCC's inlining and register allocation depend on the whole object: changing `OnEvent` changed
  whether `subString` was inlined (through estimated call frequencies) and the registers in `doGet`.
  Match a unit's biggest function before trusting its neighbours.
- Runs of single-byte `nop`s mark the gap between separate input sections (a new object or a COMDAT
  section); within one section the assembler pads with multi-byte nops (a lone 0x90 is either).
- Each object's iostream static initializer stores its own `.bss` slot, and the slots are consecutive
  in link order, which gives a unit's boundaries and its `.bss` placement.
- Register allocation follows the order local variables are declared and whether a value reuses a
  variable (`level = (level + 1) / 2` rather than a new `count`); when only registers differ, try
  declaration orders before rewriting logic.
- GCC's fold moves a plain variable to the right of `==`, so `c == table[i]` still compares
  `table[i], c`; to get the other operand order compare two locals.
- Output order moves with where a function is defined even when it is inlined everywhere: the Wave
  accessors of `CThreatLevel` sit in the Wave section of the file although Mac lists them next to
  their `CThreatLevel` wrappers.
- Spelling matters and the original is not minimal: `Size < Pos + finalPos`, a max-style
  `Size = Pos < Size ? Size : Pos`, an explicit `return CString<char>(result)` copy in `wideToAnsi`,
  early returns in `getContentLength`, a `for (;;)` state loop with per-branch `break`/`continue`.
- The original has bugs that must be kept: `doGet` returns without leaving its lock when busy,
  `wideToAnsi` frees an array with scalar `delete`, and the "Interrupted" event never sets its type.
- When a section's known symbols disagree, the earliest one anchors it and the first misplaced
  symbol shows where the lengths diverge: the function just before it differs.
- A return type can show only in other functions: `update`/`updateLogic` return `int`, not `bool`
  (the target zero-extends `setle` results), and switching it fixed five functions of
  CHarvestEntity at once, including ones that never touch it.
- GCC evaluates constructor arguments right to left, so `CPosition2d<int>((int)x, (int)y)` converts
  y first, while float screen positions built by assigning `pos.X` then `pos.Y` compute x first.
  Temporaries passed straight into a virtual call are built after the vtable load; named locals
  before it.
- Branch structure moves alignment padding: `CFindSparkFunctor::testEntity` compiled to the same
  instructions with its three tests in one `if`, but only a separate `wantsSpark` test gave the
  target's (absent) jump-target alignment.
- A wrong virtual return type can be invisible in its own unit: `onSpark` returns the spark's next
  target id (or -1, or 0), which only CSparkEntity showed.
- `CPerimeterBombExplosion` moves before damping its speed. It detonates after three seconds, or
  when a bomb that has exceeded speed 10 leaves the movable world. Alien damage falls linearly
  with squared distance within radius 200; nearby bombs receive a radial push within radius 100.
  Event 21/20 requires 20 kills; event 21/11 requires six kills and a bomb that has moved.
- `CAlienPriorities` defaults all 14 weights to 2, with range preference and hold-fire disabled.
  Hold-fire is loaded only from save version 30 onward. Its comma parser leaves unprovided weights
  unchanged, includes the terminating character in the final substring, and parses an empty final
  token as zero. The native comparison is `start <= text.size()`, not a strict inequality.
- Multi-string constructors (CMinerEntity, CSparkMoverEntity) differ only in the first inlined
  `CString` copy loop, whose compare operands are swapped (`cmp len, i; jge` in the target). Single
  string constructors match, so this is likely which inliner pass inlined that first copy.
- A unit's `.bss` can start with a variable it defines before the iostream slot:
  CSparkMoverEntity defines `g_useLargeSparkDeathParticle` there.
- `selectSparkTarget` is still open: the target fetches the next element before the loop's exit
  test (only a loop that loads it there comes close), and it keeps `this` and `excludeId` in the
  opposite callee-saved registers from ours.
- World scenery uses elliptical collision radii, with vertical distance multiplied by 1.5,
  and a 256-unit spatial grid on planet 2. Placement protects the starting area and retries up
  to four times; the first scenery choice of each planet-2 batch is forced to type 0.
- Expansion scatters minerals in 512-unit cells, reducing their count with distance from
  `(512, 512)`. The native special case resets counts from -24 through -16 to 20 before
  clamping to 2..20. Both achievement events 21/15 and 21/16 require area 10,485,760.
  The camera's oversize-height branch writes `X`, which the reconstruction preserves.

- Defense towers form directed chains. Back-target counts scale range and damage; a chain can
  reverse toward its end tower. The recovered aim-rotation and spark-refill routines match exactly,
  but the constructors and `updateLogic` still differ. A constructor with the same FDE size is not
  an exact match and receives no exact credit.
- Missile types 0, 1 and 2 are the basic missile, MIRV and Eagle. Basic blasts sort nearby aliens
  by squared distance and affect at most seven; MIRV selects three targets for tempest blasts;
  Eagle missiles refresh an entity reference and may retarget. Those flight and launch loops remain
  inexact. The acceleration/clamp routine, recursive lightning geometry, tempest damage loop,
  projectile constructors and distance-sorting helpers match exactly.
- Missile constructor integers are the owner id, missile type and target id. The target is an
  `SEntityReference` at offset 0x50; its id and update counter are not standalone kill counters.
- The entity manager has four grid layers. Building searches use layer 0 and alien targeting uses
  layer 1, whose cells begin at 0x14e8 on Linux amd64. The turret status display uses reload
  intervals of 10 seconds (basic), 28 seconds (Eagle), and 18 seconds (Tempest).
- Dropship bullets travel at 1,800 units per second and leave a beam trail capped at 120 units.
  Their impact damages aliens within 20 units with cubic distance falloff. The exact native code
  centers that damage search on the bullet's position before the impact step; it does not move
  the bullet to its target on that frame. The ship's missile and gun targets are separate entity
  references, and its collision radius is 1. The partial audio interface records virtual slot
  order; return types unused by recovered callers remain provisional.

## Inferred and merged sections

A data section with no known symbol is placed where the references to it from placed code imply,
when all of them agree, and is then compared byte for byte. References into merged string or
constant sections (for example `.rodata.str1.1`, or `.rodata.str4.4` for wide strings) are checked
by content: the string or constant at the referenced offset must equal the target's.

An inline copy (a COMDAT section) kept from another object reads that object's copy of a file-level
static, such as a header's `static const int` table, so its reference cannot land in our copy. A
reference from such a section to a local object is checked by content: the target's object at the
same place must hold our object's bytes. Both copies must be read-only: equal initial bytes do not
make mutable statics interchangeable.

Exception tables (`.gcc_except_table`) are referenced only from `.eh_frame`, which is not compared.
Each of our FDEs names its function and its table offset; the target FDE of the placed function
gives the table's address, and the section is placed when every function implies the same base.

## Per-function placement and learned symbols

Each function of an executable section is compared at its own target address, so function bodies
can match before the unit's order does; the section is exact only when every function lands at
base + offset. Functions without a known name (static initializers, GCC clones) take the target
function after the one before them when its size fits, else the one FDE of their size in the known
range, so a length difference in one function does not misplace the rest. `hv match --learn` adds the addresses of unknown symbols referenced by
functions that match everywhere else, when every such reference agrees (evidence `reloc:<unit>:<fn>`),
and of the unit's own global functions that match exactly (evidence `match:<unit>`), so units that
call them can resolve those calls.

## Definition-order search

For bounded definition-order experiments, see [Definition-order search](search.md). `hv search`
uses explicit source blocks, preserves existing exact matches, saves a candidate patch and verifies
an improvement before optional application.

## Shared headers

A recovered unit declares what it uses from classes it does not own (for game code these are mostly
`CWorld`, the entity classes, `CSystemConfig` and the event list) in the header the Mac debug map
names for that class, marked partial:

- only the members and functions the recovered units use, with virtual functions in vtable order
  (their names and order come from the ported vtables);
- sizes that matter to a caller (for example, a caller allocating a not-yet-recovered class) kept by an explicit
  `Unrecovered` byte array until the owning unit is recovered;
- file-level statics that every includer defines (`ENERGY_PROGRESS_COLOR`, the 4096.0 grid offset)
  in a header, because each object initializes its own copy in its static initializer.

The owning unit replaces the partial declaration when it is recovered. Game sources include from
the source roots (`-Isrc -Isrc/HarvestFull`, as `ox/...` and `harvest/...`).
