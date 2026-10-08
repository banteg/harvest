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
- Thunks are skipped here: their Mac names encode the i386 `this` adjustment. `just extents` finds
  and names the Linux thunks instead (below).

Rows with other evidence (for example `manual:`) are kept when the generator reruns.

The checked GCC 4.4 this-adjusting thunks have no FDE. `just extents` (`hv extents`) proves their
extents another way and writes `config/<build>/extents.tsv`. RTTI identifies primary and secondary
vtable headers; contiguous function slots after each header supply candidates. The walk stops at
the next header, RTTI object or non-code word, and permits null abstract-destructor slots.

Only the observed operand shapes qualify: `add rdi, negative immediate; jmp` (`rsi` when a hidden
return pointer occupies `rdi`), or `mov r10, [rdi]; add rdi, [r10 + negative aligned offset]; jmp`.
The jump must be direct and land at an FDE start. Thunk extents overlap neither FDEs nor each other;
named thunks missing this proof fail generation. Each row records the vtable slot and jump target.
Names use the Itanium mangling of the adjustment and target (`_ZThn24_...`, `_ZTv0_n24_...`), which
reproduces every thunk name written by hand before. New names go into `symbols.tsv` with `thunk:`
evidence; candidates with unnamed jump targets are flagged. No unwind entries are synthesized.

Matching, searching and progress capture verify the exact loaded image bytes against the pin,
regenerate the table in memory, and require it to reproduce the committed file before accepting
its boundaries. Editing a boundary, name or slot therefore requires `just extents` again. The
regeneration test checks this against the original; synthetic tests reject forged tables, wrong
operand roles, invalid jumps and overlapping extents.

Mac virtual-slot names need instruction-level validation when the layouts differ. Linux's
`CFileSystem` inserts a virtual at slot 12, shifting subsequent Mac names by one. Manual rows
correct those shifted names. The inserted method's original name is unavailable; it drops every
cached zip reader, and the recovered interface calls it `dropZipReaders`. `existFile` is Linux
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
executable sections, each function must also start an FDE of exactly its size in the target, or a
thunk extent from `extents.tsv`, which pins the extent of the last function too. Each function row
records which one proved its extent (`"extent": "fde"` or `"thunk"`). A unit is exact when every
placed section matches and none is left unplaced. Reports go to `build/match/<build>/<unit>.json`:
placements, bad references, differing bytes, per-function results, and compilation provenance
(container image ID, compiler version, package manifest hash, command, and hashes of every
repository input from GCC's dependency file).

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
PLT, copied library data, our placed sections and functions by offset; `sub_`/`lbl_` otherwise). Calls to a local
function in the same section are resolved in place, as the assembler did for ours. Referenced strings,
narrow or wide, that our object also has go into a target copy of our merged string section at the same offsets,
holding the target's bytes. Nothing is copied from our relocations, so a wrong target stays visible.
Indexed absolute operands also get relocations. Binary merge elements are checked by the memory
operand's access width and content, separately from strings; non-allocated metadata such as
`.comment` cannot supply literals.
With these objects objdiff scores every function of the exact units at 100%, matching `hv match`.

## Tests

- `uv run pytest`: synthetic ELF fixtures (call destinations and addends, unknown symbols,
  unsupported types, overflow, overlaps, changed constants, FDE extents, BSS bounds and placement
  conflicts, mutable static substitution, indexed addresses, binary literals, wide strings and
  jump table entries); with the originals present, RTTI and vtable porting.
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
| `ox/net/CHTTPConnectionHandler.cpp` | 20/23 | `OnEvent` matches once the unit's `CString<char>::append` copy is placed; that copy differs by one compare operand order in its copy loop; function order differs |
| `ox/net/CVariablePacket.cpp` | 32/32 | packet parser and builder; function order differs |
| `ox/io/CHelpIO.cpp` | 19/19 | Complete unit: numeric and string I/O, free-filename selection, static initializer; all compared sections match |
| `ox/algo/CRand.cpp`, `CSimplePress.cpp`, `CTimeCounter.cpp` | 35/35 | exact |
| `ox/core/CBasic.cpp`, `CCipherKey.cpp`, `CCriticalSection.cpp`, `CHiddenFloat.cpp`, `CHiddenInt.cpp`, `CThread.cpp` | 61/61 | exact |
| `daisy/video/Null/CFPSCounter.cpp` | 5/5 | exact; Irrlicht 0.7 FPS calculation, both constructors and the iostream initializer |
| `HarvestFull/harvest/game/CThreatLevel.cpp` | 79/81 | game modes, waves and attack spawning (clamped wave and bonus terms); `spawnAliensInArea` and `alienOccursOnPlanet` differ only in branch and case block placement |
| `ox/entity/COxEntity.cpp` | 25/25 | exact; keeps the 2d constructors' `Position.Y` typo |
| `HarvestFull/harvest/entity/CBuildingEntity.cpp` | 31/31 | exact; the building's Lua view (Lunar method table) |
| `HarvestFull/harvest/entity/CSparkProducerEntity.cpp` | 28/28 | exact; the solar collector |
| `HarvestFull/harvest/entity/CSparkMoverEntity.cpp` | 38/40 | energy links: waypoints, heat, overcharge; audited (a NaN heat takes the overheated path, as in the build); only the C1/C2 constructors remain, with swapped operands in the inlined strlen loop |
| `HarvestFull/harvest/entity/CMinerEntity.cpp` | 33/35 | the mineral harvester and its energy beam (the beam fades below `4.2f - 0.5f`, the build's constant); only the C1/C2 constructors remain, with swapped operands in the inlined strlen loop |
| `HarvestFull/harvest/entity/CConstructionEntity.cpp` | 30/30 | exact; construction sites and calling spark movers |
| `HarvestFull/harvest/entity/CMineralsEntity.cpp` | 33/33 | exact; mineral deposits and the level scatter |
| `HarvestFull/harvest/entity/CSparkEntity.cpp` | 27/27 | exact; a spark homing on its target building through an `SEntityReference` |
| `HarvestFull/harvest/entity/CHarvestEntity.cpp` | 69/70 | `CEntity`, particles, special effects, spark search; `selectSparkTarget` keeps loop layout and register differences; its behaviour is verified against the target |
| `HarvestFull/harvest/entity/CPerimeterBomb.cpp` | 24/25 | three-second fuse, radial alien damage, bomb knockback and kill events; the explosion's `updateLogic` has swapped list-iterator compare operands |
| `HarvestFull/harvest/settings/CAlienPriorities.cpp` | 11/11 | weapon targeting weights, range preference, hold-fire and serialization; destructor section placement differs |
| `HarvestFull/harvest/entity/CDefenseTowerEntity.cpp` | 46/50 | the laser defense tower: linking, targeting, beam update and sprite setup. The constructors differ only in which `CString` temporaries GCC inlines (19/107 against our 20/106), and `updateLogic`/`updateTowerLinks` only in block layout and register allocation; no remaining semantic differences |
| `HarvestFull/harvest/entity/CMissileTurretEntity.cpp` | 73/77 | the missile turret, missiles and tempest blast; audited (Mirv and Eagle set their launch speed and height only with a particle package, as in the build); the turret's `updateLogic` (block layout), `CMissileEntity::updateLogic` (registers and sort placement), `findPriorityAlien` (registers) and `render` (scheduling) remain |
| `HarvestFull/harvest/entity/CAlienEntity.cpp` | 59/67 | alien layout, constructors, save/load, Lua control, damage, all movement AI, target search, rendering, update logic and sprite sets, checked against the target; inexact: `updateSprite` registers, miner movement layout, two iterator compare operand orders in `locateTargetBuilding`, `render`/`updateLogic` scheduling, and three large sprite setups whose `CString` inlining choice follows GCC's unit-wide inline order |
| `HarvestFull/harvest/entity/CDropshipEntity.cpp` | 53/55 | the dropship and its bullet: flight state machine, missile salvos, landing and takeoff, engine sound. `updateLogic` has the target's instructions in a different block layout, and `loadSprite` (98%) inlines one `CString` constructor fewer because the original unit was slightly larger (the inline-unit-growth budget); no remaining semantic differences |
| `HarvestFull/harvest/entity/CShuttleEntity.cpp` | 42/44 | race placement and sorting, steering AI, checkpoints, rendering and state accessors; audited (`resetAllAiByMe(copyMine)` had its branches inverted); `resetAllAiByMe` differs only in which registers hold the hoisted %14/%51 constants, `updateLogic` only in where block reordering places the cold checkpoint fix-up and arrival blocks |
| `HarvestFull/harvest/entity/CCreativeEntity.cpp` | 54/55 | creative buildings, Lua energy/color/progress/sprite controls, private-table serialization, constructors and rendering; `loadBuildingData` has a `CString` copy-loop operand swap |
| `HarvestFull/harvest/entity/CEntityManager.cpp` | 41/43 | building lists and bounds, spatial grid maintenance, cached targeting searches, entity construction, save/load, and energy-beam rendering; `isBuildingPlacementOk` (the iterator lives in memory instead of a spilled register) and `findAllGridEntitiesInRange` (registers) remain; the constructor matches once `getBuildingsBoundingBox` is defined before it, as `hv search` found |
| `HarvestFull/harvest/gfx/CScatterShader.cpp` | 16/17 | planet scattering shader: construction, Cg material setup and per-frame shader constants with inlined `CMatrix4` math; the base-object constructor differs only in store scheduling |
| `ox/entity/COxEntityManager.cpp` | 20/21 | entity ownership, deferred spawning and removal, reference refresh, first/best/all targeting searches, and render ordering with native sort helpers; the update loop remains inexact |
| `HarvestFull/harvest/game/CWorld.cpp` | 34/38 | complete world source: scenery collision grid, wind forces, sprite loading, scenario startup, edge shading, camera bounds and serialization; audited against the target (the world-area expansion events fire only in game mode 0). `update`, `expandWorld`, `placeDoodads`, `findRendezvousPoint`, `renderBackground` and the two collision tests differ only in scheduling, spills, stack slots and block layout |
| `HarvestFull/harvest/game/CStatistics.cpp` | 41/41 | wave summaries, protected score totals, event logs, versioned serialization and highscore accessors; exact. Only the per-level accumulation is guarded by a current level; rush damage and the game-wide stats update regardless, as in the build. The highscore XOR scrambling is an inline helper (our name) |
| `HarvestFull/harvest/game/CScenario.cpp` | 63/64 | complete campaign source: the 200-call starting map, lifecycle, dialogue, dropship landing and departure, mineral-triggered reinforcements, completion conditions and event callbacks; `update` is exact; `createScenarioEvents` differs only in which credit-dialogue constructor calls GCC inlines |
| `HarvestFull/harvest/game/CLuaManager.cpp` | 181/204 | the harvest Lua library (61 bindings), hooks, mods, Lua save values and the Lunar templates; audited against the target (`renderSpriteStateFreeShape` takes the corners from arguments 2-9 and the y from argument 10). The remaining misses are GCC's unit-wide inlining choices for `CString` copies and assignments (creative hooks, save values, mod lists, vector templates; the unit hits inline-unit-growth exactly), register and operand-order choices, block layout in `findBuildings` and `isPositionBlocked`, and a sign-extension order in `hookWaveButton` |
| `HarvestFull/harvest/settings/CSystemConfig.cpp` | 46/48 | settings file accessors, language loading, the out-of-line `getCurrentLanguage` (as on Mac), `applySettings`, `getLocalizedText` and `replaceAll<wchar_t>`; the constructors match with the missing-resolution branch first; `getAllLanguages` and `replaceAll` differ only in compare operand order in inlined `CString` loops; `setRecentProfile` and `addLicenseKeySetting` match once `setRecentProfile` is defined after `getScrollSpeed`, as `hv search` found |
| `HarvestFull/harvest/settings/CSavestateInfo.cpp` | 3/3 | exact; save game header read/write |
| `HarvestFull/harvest/states/CIntroState.cpp` | 12/13 | the splash state: step-wise loading (all 37 steps are explicit cases), fades, layer slide and rendering; `OnEvent` has every block of the target but lays out the mouse branch's skip logic with the opposite branch sense |
| `HarvestFull/harvest/states/CLoadingScreen.cpp` | 8/8 | the spinning logo and version line drawn while states load |
| `HarvestFull/harvest/gui/CIngameMenuScreen.cpp` | 10/10 | the in-game menu: button frame, abandon-game confirmations and the shared `sendCustomEvent` helper; exact; `OnEvent` switches on the GUI event type; audited against the target |
| `HarvestFull/harvest/gui/CMenuInfoDialog.cpp` | 13/13 | the tutorial briefing dialog: voice lines, typing dots, the start/close buttons and `refillMainView` (portrait texts, progressive reveal timing); exact; `OnEvent` switches on the event type and the button id; audited against the target |
| `HarvestFull/main.cpp` | 2/2 | the program entry running the `CHarvestFullMain` loop |
| `HarvestFull/harvest/CHarvestFullMain.cpp` | 19/19 | game-wide globals (`g_loadGameFilename` starts as `""`), device and settings start-up (`init`; the Linux build has no Steam), the state factory, the shared input receiver (button sounds tested before the checkbox, Ctrl+T screenshots, fullscreen toggle) and teardown; its `vector<CString<wchar_t>>` insert copy has been sensitive to the declaration count of shared headers |
| `HarvestFull/harvest/gui/CSettingsScreen.cpp` | 20/22 | the settings window: general tab (audio sliders, 32-bit resolution list of at least 800x600, fullscreen, particle and scroll-speed sliders, language list) and key-binding tab, control helpers, screen-mode change and key rebinding; `OnEvent` differs in block layout and registers, the `TArray<SLanguageFile>` destructor by one alignment nop; audited against the target |
| `HarvestFull/harvest/gui/CGuiEffects.cpp` | 3/3 | the screen-darkening overlay with holes around entities of one type (`renderRecangleOverlay`) and its rectangle splitter |
| `HarvestFull/harvest/gui/CGuiInfoLines.cpp` | 9/9 | the in-game message list: lifetime, `update` and both `addInfoLine` overloads |
| `HarvestFull/harvest/gui/CAchievementsScreen.cpp` | 19/20 | the award board: layout, rating title, award tooltips, the Medusa dialogs and the wobbling wicked-awesome award, with the award position and Medusa text tables; the only miss is this unit's copy of the `CString<char>::operator=` COMDAT (one register), whose kept copy belongs to another object |
| `HarvestFull/harvest/gui/CPriorityScreen.cpp` | 17/20 | the alien priority screen: priority boxes dragged between rows, sprites, target-closest/hold-fire boxes and saving to the profile; `OnEvent` needs the `T.326` clone (resolves only with the full .text order) and places the checkbox loop differently; `updatePriorityBoxPositions` has two `lea`/`add` choices; plus a `CString` copy; audited against the target |
| `HarvestFull/harvest/states/CShuttleRaceState.cpp` | 31/34 | the shuttle race minigame state (bases `CGameState`, `IScenario`, `IParticleEngineCallback`): OnEvent, particle callbacks, firstInit, lifetime, the `LAP_CHECKPOINTS` initializer and the `CString<wchar_t>::append` copy; `updateState` differs by a copied loop header (the target leaves the 10 ms step loop uncopied), one load/store swap and clamp registers; `secondInit` has every case body of the target in a different block order; `render` differs by registers only |
| `HarvestFull/harvest/entity/CBuildableItems.cpp` | 32/33 | the buildable buildings list: standard and creative loaders, `addBuilding` (4.8 KB), special upgrades, lookups, button rendering and vector helpers; audited (wrapping forward over an empty list loops, as in the build); `loadCreativeBuildingList` differs only in register allocation |
| `HarvestFull/harvest/settings/CHarvestProfile.cpp` | 43/47 | the profile settings file: name and group, local scores, achievements, key mapping and weapon priorities, plus the `splitString<wchar_t>` copy; `getAchievementSpriteName` (register and stack-slot choices), `getFilename` and `parseLocalAchievementsString` (registers in inlined `CString` copies) and the shared `vector<CString<wchar_t>>` insert helper remain |
| `HarvestFull/harvest/settings/CProfileManager.cpp` | 17/17 | the profile list: open, create and delete, and `gp_profileManager`; `createProfile` and `createProfileList` match in the definition order `hv search` found, and function placement still differs |
| `HarvestFull/harvest/gui/CProfileScreen.cpp` | 17/18 | the profile list and create/edit windows, name validation and trimming; only `saveProfile` remains, with its two cold null-string blocks in a different order; audited against the target |
| `HarvestFull/harvest/gui/CStatisticsScreen.cpp` | 21/24 | the post-game statistics screen: wave graphs with fade-in, event log and totals with paging, local bests, highscore upload over HTTP and the window/graph draw handlers; `OnEvent` (the target inlines the `CString` assignment and peels the hover loop), `createGraphs` and `renderGraph` (scheduling, a register-held width) remain; all match the target's behaviour; audited against the target |
| `HarvestFull/harvest/gui/CStoryScreen.cpp` | 15/16 | the campaign dialogue letterbox: two bordered lines with portrait, name and progressively revealed text, fades, voice playback and the skip key; in `OnEvent`, GCC compares `id` before `EventType`, the reverse of the target; audited against the target |
| `HarvestFull/harvest/gui/CHighscoreScreen.cpp` | 26/33 | the online highscore screen: summary pages with promote buttons, highscore table parsing, HTTP requests, Base64/UTF-8 name coding and filter popups, plus the `splitString<char>` and `CString<wchar_t>` assignment COMDATs; `parseHighscoreString` (continue layout, tied to the unit's inline budget), `OnEvent`, `setVisible`, `createStatusString`, `update`, `parseSummaryPage` (registers and scheduling only) and the `_M_insert_aux` copy remain; audited against the target |
| `HarvestFull/harvest/gui/CSaveGameScreen.cpp` | 31/36 | the save/load screen: the profile's save games sorted with `std::sort`, delete/overwrite message boxes, description box and mode/planet icons (`SSaveListItem`, `SSavestateHeader`); only template copies remain: the `SSaveListItem` sort helpers (copy-constructor inlining) and a `CString<char>::operator=` copy (one register); audited against the target |
| `HarvestFull/harvest/states/CMainMenuState.cpp` | 28/32 | the main menu state: planet scene, camera fly-over, console buttons, planet and game-mode selection, mod and demo dialogs; the heading reset builds `CString<wchar_t>("")`, whose IPA-CP clone `T.548` matches; `OnEvent` and `secondInit` differ by block layout and registers, `enterGameModeSelectMode` by where the inlined heading string stores its array, and the `CString<char>::operator+=` copy by one swapped compare |
| `HarvestFull/harvest/states/CPlayState.cpp` | 73/98 | the in-game state: GUI setup, input, rendering, minimap, save/load, wave and creative lists and the game loop. `OnEvent`, `updateState`, `renderMinimap` and `newSelectedEntity` were checked against the target and do what it does, but differ in block layout, register allocation, stack slots and float scheduling; `readStateFromFile` is exact. `buyBuildingAtPlacementPos`, `updatePlacementPosition`, `secondInit`, `initializeNewGame`, `writeStateToFile` and `setWaveListToggle` differ only in operand order, loop rotation or registers, and three `CConditionalHarvestEvent` copies come from `CScenario.h` declaring `runEvent` out of line but defining it `inline`, which makes it the key function in every includer. The `.rodata` pin is where the unit's jump tables start |
| `daisy/video/Null/CParticlePackage.cpp` | 28/31 | the particle package reader (format versions 0-8: type info, waveform functions, '/'-split name lists), types sorted with `std::sort` and found by `ox::algo::binarySearchIf`, importance filter, state creation and removal; `__insertion_sort`/`__introsort_loop` (swapped compares in the inlined `CString::operator<`) and this unit's `vector<CString<char>>` insert copy remain. Defining `load` after its helpers made four functions exact at once |
| `daisy/video/Null/CParticleState.cpp` | 29/30 | particle behaviour: random animation, start speed and direction, lifetime, colour and scale fades, eight waveform types, bounces, wind, start sound, timed pulses, on-die particles chosen by '@' marker groups, 2D/shadow/3D rendering; only `update` (94%, registers, one compare and two block placements) remains. Keeps the original's reversed direction/elevation ranges (`min - max`) |
| `daisy/audio/CAudioDriver.cpp` | 67/70 | the sound logic over the OpenAL backend: volume settings as gains, voice ducking, tracked-sound handles, string-keyed sound and music maps, and the device hooks the backend overrides; `findOrLoadSound`, `loopSound` and `playMusic` differ only in register allocation and where string-destructor cleanups sit |
| `daisy/gui/CGUIFont.cpp` | 20/22 | Irrlicht's bitmap font with Oxeye changes: character pixels keep their texture colours, right and bottom alignment, `std::vector` storage; both constructor copies are byte-identical but call the local `reserve` clone `T.189`, whose address depends on the unit's .text order, which is not reproduced |
| `daisy/gui/CUnicodeFont.cpp` | 17/17 | exact; the sprite-package font: characters are animations named by decimal code, kept in 1999 hash buckets and loaded on first use, with `#0`-`#9` and `#o` colour codes |
| `daisy/gui/CGUIFileOpenDialog.cpp` | 55/61 | file chooser built from a layout window (directory and file list boxes, Open/Cancel, the path text) that sends `EGET_FILE_SELECTED` (13) or `EGET_FILE_CHOOSE_DIALOG_CANCELLED` (14) to its parent; all own functions are exact except `OnEvent` (one `CString` operand swap). Like every widget unit, the count includes inline `IGUIElement` copies and `_ZThn24_` thunks |
| `daisy/gui/CGUIImage.cpp` | 49/54 | texture or sprite-animation image with an override colour; every function of its own is exact, only shared `IGUIElement` inline copies differ |
| `daisy/gui/CGUIInOutFader.cpp` | 50/54 | Irrlicht's fader using `SColor::getInterpolated`; every function of its own is exact |
| `daisy/gui/CGUIListBox.cpp` | 64/74 | Oxeye's list box: items are children of a layout group inside a frame or layout group, with a scroll bar, static-text items wrapped by `IGUIStaticText::getMultilineHeight`, selection events to an override action parent and a bordered highlight. GCC orders the object's functions differently from the target (`addTextItem`, `setTextItemIndent`), which breaks the constructor copies; the destructors match since `setIconFont` is declared only in `CGUIListBox`, as in the original's vtables, where `IGUIListBox` has 15 pure virtuals; `draw` matches with its `DrawBack` test first; `selectNew` and `addTextItem` differ by registers and layout |
| `daisy/gui/CGUIMenu.cpp` | 42/46 | Irrlicht's menu bar over the context menu; only `updateAbsolutePosition` differs, by registers |
| `daisy/gui/CGUIButton.cpp` | 63/77 | `CGUIButton` (sprite-state buttons, close-button text 0x103) and `CGUITextButton` share this object, plus the `IGUIButton`/`IGUITextButton` inline destructors; `OnEvent` and `CGUITextButton::draw` differ by one operand order, `draw` places the resize block out of line, and the constructor/destructor exception cleanups inline `~IGUIElement` where the build calls it. The rest are `IGUIElement` copies kept from CGUIEnvironment |
| `daisy/gui/CGUICheckBox.cpp` | 54/59 | sprite-skinned check box with text font and colour, `updateWidth`, and the `IGUICheckBox` destructors; every own function is exact |
| `daisy/gui/CGUIClickArea.cpp` | 54/66 | `CClickArea`, an `IGUIClickArea`/`IGUILayout` element that turns mouse buttons into `EGET_CLICK_AREA_*` events (31-36); every own function is exact |
| `daisy/gui/CGUIComboBox.cpp` | 58/64 | the combo box (background and arrow sprites, list opened as a `CGUIListBox` on the root element, at most 10 rows) and the `IGUIComboBox` destructors; every own function is exact; the `IGUIElement` copies and `vector<CString<wchar_t>>::_M_insert_aux` are kept from other objects |
| `daisy/gui/CGUIEnvironment.cpp` | 129/155 | the GUI root (a layout) and widget factory: focus, hover with hover descriptions, mouse and key dispatch, fonts in a sorted vector (`.fnt` files load as `CUnicodeFont`), the built-in font and the skin. The object also holds the first copies of the inline ox code (`IGUIElement` methods, the `IGUILayout` sorters from `IGUILayoutInline.h`, `IGUIStaticText::breakText` (from `IGUIStaticTextInline.h`, which this unit includes), `IGUIHoverParent`). Remaining: register and operand order in `CString` loops and sort helpers, destructor inlining that follows the unit budget, and block layout in `sortRiver`, `breakText`, `sortFlow` and `postEventFromUser`; including `breakText` here costs one `std::__insertion_sort<SFont>` copy (one register) but gains 21 functions |
| `daisy/gui/CGUISkin.cpp` | 21/21 | exact; the default skin: Irrlicht colours made opaque plus Oxeye's modal shade and list highlight fill and border, radio-button sizes, message box texts, and a sprite package that sets the button, scroll bar and check box sizes |
| `daisy/gui/CGUIContextMenu.cpp` | 64/74 | Irrlicht's context menu with items in a `std::vector<SItem>` and its vector copies; `sendClick`, `highlight` and `removeItem` differ by compare operand order, the cleanup-path `~IGUIElement` inline sits in C2 instead of D2, and the destructors keep `Items` in registers where the build reloads it |
| `daisy/gui/CGUIEditBox.cpp` | 69/76 | Oxeye's edit box: sprite background and caps, hidden text, Tab to the next edit box, Enter clicks the associated button, change events, paste through an `EKIE_PASTE` key event, no shift selection or copy/cut; the constructor, destructor and `ansiToWide` copy match with `getCursorPos` defined right after the constructor, as `hv search` found, and `processKey` (98.7%) and `draw` (99.99%) differ by registers, operand order and landing-pad layout only |
| `daisy/gui/CGUIStaticText.cpp` | 55/69 | Irrlicht's static text with Oxeye's alignment, paragraph icon (`"\|right"` suffix), progressive reveal at 300 px/s and scrolling of overflowing centred text; the destructor matches in the definition order `hv search` found; remaining: constructor cleanup inlining (unit budget), registers and block order in `draw`, `breakText` and `getPreferredSize` |
| `daisy/gui/CGUITabControl.cpp` | 99/122 | `CGUITab`, `CGUITabControl` (Irrlicht drawing plus a 10-piece sprite skin) and Oxeye's `CGUITabButtonRow` (overlapping tabs, close button) share this object; the `IGUITabControl` and `IGUITabButtonRow` destructors match in the definition order `hv search` found; remaining: destructor and constructor cleanup inlining that follows the inline layout bodies, and register/layout differences in both draws, `setAnimations` and the row's `OnEvent` |
| `daisy/gui/CGUIToolBar.cpp` | 45/53 | Irrlicht's toolbar without button images; `updateAbsolutePosition` differs by a hoisted load, the constructors by one register swap |
| `daisy/gui/CGUIWindow.cpp` | 88/111 | Irrlicht's window with Oxeye's 9-part sprite frame, inner edges and frame-only mode, plus `CGUIDetachableFrame` (fading, double-click locking, resize handle, options popup, snapping to layout groups within 14 px) in the same object; behaviour checked against the target. The `OnEvent`s and `snapToItem` differ by block layout, both draws, `lockToItemInside` and `updateButtonPositions` by registers or operand order |
| `daisy/gui/CGUIMeshViewer.cpp` | 51/56 | Irrlicht's mesh viewer over the ox mesh interfaces; every own function is exact (it showed `SMaterial`'s defaults: white diffuse colour, textures cleared after the flags) |
| `daisy/gui/CGUIMessageBox.cpp` | 36/42 | the message box with a button layout group and return/escape answers; every own function is exact |
| `daisy/gui/CGUIModalScreen.cpp` | 56/69 | the modal screen that tints the screen and reports blocked input; `updateAbsolutePosition` differs only in scheduling and registers |
| `daisy/gui/CGUIPopupMenu.cpp` | 50/55 | Oxeye's popup menu (frame, title, options, highlight using the list highlight colours); every own function is exact |
| `daisy/gui/CGUIRadioList.cpp` | 52/59 | Oxeye's radio list of check boxes; C1 and D1 inline the `IGUIElement` destructor in their exception cleanup where the build calls it |
| `daisy/gui/CGUIScrollBar.cpp` | 60/66 | the sprite-skinned scroll bar with paging, thumb states and `setAnimations`; `OnEvent` differs only in when the mouse position is loaded |
| `daisy/io/CZipFileList.cpp` | 16/18 | the zip folder listing (filter and directory constructor, getters, `FileEntry` vector copies); both constructor copies differ by one swapped `cmp` operand pair |
| `daisy/io/CZipReader.cpp` | 24/30 | the zip reader: local-header scan with data-descriptor search, stored and inflated open, linear folder+name lookup, `directoryExists` (whose loop never advances; see `docs/port/zip-archives.md`), `SZipFileEntry` sort and vector copies; `scanLocalHeader` and five std sort/vector templates differ by register allocation only |
| `daisy/io/CFileSystem.cpp` | 48/59 | the file system: aliases (`intern_resolveAliases`), zip-extension and zip-reader maps, paths into archives, changing directory into archives, folder and archive file lists, `existFile`, zlib inflate/deflate, the read/write/memory file factories, the inline `CFilePath` and the map copies; nine methods differ by a swapped `CString::operator<` operand in an inlined `lower_bound` or by registers, as do two map copies; audited against the target (rules in `docs/port/file-system.md`) |
| `daisy/io/CMemoryReadFile.cpp` | 10/10 | exact; the memory read file with a file name, and `createMemoryReadFile` |
| `daisy/io/CReadFile.cpp` | 19/19 | exact; the stdio read file (`fgets` `readLine`, `fstat64` modified date) and `createReadFile` |
| `daisy/io/CWriteFile.cpp` | 17/17 | exact; the stdio write file (`ab`/`wb`) and `createWriteFile` |
| `daisy/io/CFileList.cpp` | 18/18 | exact; the glob(3) folder listing with `GLOB_MARK` directory detection and the all/files/directories modes; the constructor is defined last to match the C1/C2 order |
| `daisy/io/CLimitReadFile.cpp` | 19/19 | exact; the window over a file that serves stored zip entries, and `createLimitReadFile` |
| `daisy/other/CIrrDeviceLinux.cpp` | 54/57 | the SFML/X11 Linux device: key table, mouse, text and joystick translation, double clicks, window creation, resize clamping, fullscreen switching, the mode list and `$GAME_RESOURCES$`; `run`, `postMouseEvent`, `createUserSelectedDeviceWindow` and `setFullscreenMode` differ in block layout, compare order or scheduling only (rules in `docs/port/input-and-window.md`) |
| `daisy/other/CIrrDeviceStub.cpp` | 59/63 | the device base: subsystem ownership, GUI-first event routing, version check, drag-and-drop and named network devices, lazy audio and joystick drivers; four functions differ in `CString` scheduling or operand order; `CWinsockNetworkDevice` is a provisional sized declaration |
| `daisy/other/CLogger.cpp` | 17/17 | exact; the stdout logger: level threshold, "text: hint", printf/wprintf logging, `logWithInfo` |
| `daisy/other/CLinuxOperator.cpp` | 23/23 | the OS operator: GTK clipboard, `xdg-open`, `$HOME` paths and empty stubs; the two path functions match in the definition order `hv search` found |
| `daisy/other/os.cpp` | 9/9 | exact; the `os::Printer` forwarders and the `gettimeofday` timer |
| `daisy/input/CJoystickLinuxDriver.cpp` | 15/15 | exact; the `sf::Joystick` driver |
| `daisy/input/CJoystickNullDriver.cpp` | 12/12 | exact; the null joystick driver |
| `daisy/audio/COpenALDriver.cpp` | 53/54 | the OpenAL backend: 32-source pool, tracked sounds, Ogg/WAV effects, 10×4 KB Vorbis streaming; `_loadOggFromMemory` has two locals in swapped stack slots |
| `daisy/video/CVideoModeList.cpp` | 21/22 | the sorted video mode list; `std::__introsort_loop` differs in registers only |
| `daisy/video/Null/CVideoNull.cpp` | 154/185 | the driver base: texture, sprite-package, particle-package and material-renderer caches (sorted vectors keyed by lowercased name), image loading, render and physical screen size with the 2D view values, FPS and statistics, shader-from-file loading, post-processing surfaces, Irrlicht's 3D debug drawing and Oxeye's Bezier/Hermite helpers; remaining: std sort, heap and insert copies (GCC inlines `CString::operator=` differently across the unit; defining `setInputTexture` after `createDeviceDependentTexture` matches the `SSurface` `__final_insertion_sort`) and registers in `getSpritePackage`, `getParticlePackage` and `addTexture`; the unreached helpers differ only in scheduling (rules in `docs/port/renderer.md`) |
| `daisy/video/OpenGL/CVideoOpenGL.cpp` | 165/177 | the Linux OpenGL driver: 2D batching (1024 quads), the four `draw2DImage` variants, lines and rectangles, the 2D/3D render-state switch, material renderers, lights, stencil shadows, scissor, extension loading and the JPEG screenshot; remaining: case order in `queryFeature`, scheduling in the `draw2DImage` variants, `switch2dRendering` and `loadExtensions`, block layout in `setRenderStates3DMode`, and `CString` inlining in `saveJpegScreenshot` (rules in `docs/port/renderer.md`) |
| `daisy/video/OpenGL/COpenGLTexture.cpp` | 20/20 | exact; power-of-two RGBA8 upload, linear filters, mip maps only with flag `0x10` |
| `daisy/video/OpenGL/COpenGLShaderMaterialRenderer.cpp` | 19/19 | exact; ARB vertex and fragment programs (the fragment program is bound with the vertex program's name, an original bug) |
| `daisy/video/OpenGL/COpenGLSLMaterialRenderer.cpp` | 39/42 | GLSL program objects and the uniform list; the destructors differ by one register swap in the uniform-list clear |
| `daisy/video/OpenGL/COpenGLCGMaterialRenderer.cpp` | 13/13 | Cg program binding per material change |
| `daisy/video/Null/CCGMaterialRenderer.cpp` | 33/36 | Cg program compilation, variable setting and error logging; the logging helpers differ in `CString` compare order, and the destructor variants are emitted in reverse order |
| `daisy/video/Null/CSpriteAnimationState.cpp` | 32/32 | exact; a running sprite animation: frame advance with per-frame jumps and millisecond durations (negative holds for 48 h), save/load, and per-frame forwarding to the shared images |
| `daisy/video/Null/CSpriteAnimationImageBundle.cpp` | 26/27 | a bundle drawn as a grid of sprite images (only the plain draw is implemented); `getSize` differs in register allocation |
| `daisy/video/Null/CSpriteAnimationImage.cpp` | 22/26 | one sprite image drawn plain, scaled, mirrored, rotated, free-shape, with corner or per-vertex colours, or as a 3D quad; all values match the target. `drawRotated` and `drawMultipleColors` differ in scheduling, `drawMirrored` in stack slot order and an alias reload of `position`, and `draw3d` (with the target's per-vertex `mirrored ?:` texture coordinates) in material store order and registers |
| `daisy/video/Null/CSpritePackage.cpp` | 68/71 | the `.dat` sprite package format (header, lazily decoded texture planes, sprites, bundles and animations; layout in `CSpritePackage.h`), id-sorted image and state lists with binary and linear search; definition order follows the Linux layout. `load` differs in registers, loop rotation and one `~SAnimationData` inlining choice, and two `vector<CString<char>>` copies owned by CLuaManager differ too; `removeAnimationState` matches in the definition order `hv search` found. Sprite and bundle records must stay plain old data (the shared header is a member, not a base) or libstdc++ stops using `memmove` and every sort and vector template changes |
| `ox/game/CConfigBlock.cpp` | 22/23 | a named block of (name, value) `CString` attributes: lookup, typed get/set overloads and `printBlock`; only `getAttribute(CString)` has one swapped compare in the inlined `CString::operator=` self-check |
| `ox/game/CConfiguration.cpp` | 34/39 | the configuration file format used for settings, profiles, languages and mods: `name { attr = value; }` blocks with `#` comments, backslash-escaped `;`, 0xfeff-marked 16-bit files (else locale text), base64 attributes. `getBlock(const wchar_t*)` is defined before `blockExists(const wchar_t*)`, which gives the build's registers. Four functions differ only in string-length-loop registers or one compare order; `parseString` has the target's control flow, calls and accesses but not its block layout |
| `ox/event/IEventReceiver.cpp` | 17/17 | event receivers and the global subscriber list: subscribe, unsubscribe and delayed events buffered twice under a critical section, with the two `vector::_M_insert_aux` copies |
| `ox/game/CGameMain.cpp` | 15/15 | the main loop: creates the subscriber list, switches states, steps frames by elapsed time, sleeps while inactive, runs delayed events, and reports missing data files in a message box |
| `ox/core/AlwaysAppendToFile.cpp` | 2/2 | `vsprintf` into a 1 KB buffer appended to a log file (the unit name is ours) |
| `ox/core/CAes.cpp` | 29/30 | AES-256 used for save scrambling: key expansion, GF(2^8) tables, block encrypt/decrypt with a zero-padded last block, and the `ICipher` defaults; `expandKey` differs by one swapped loop compare |
| `ox/algo/CBase64url.cpp` | 4/5 | base64url encode with `=` padding, `encode2` (standard alphabet with `+`, `/` and `=` percent-encoded) and decode through a `-`..`z` table; `encode2` differs only by two swapped compares in inlined `CString` appends |
| `ox/gui/IGUIElement.cpp` | 39/43 | `remove` is the key function, so the unit holds the element vtable and destructors; it also includes `IGUIElementInline.h`, the virtuals after `remove()` (draw, move, visibility, `OnEvent`, `bringToFront`, ...) and the constructor, which the original kept inline as Irrlicht does. Their first copies are defined here; `removeChild`, `bringToFront`, `setText` and `updateAbsolutePosition` differ because the target's first copies sit in CGUIEnvironment, where GCC called `erase`, `drop` and the `CString` assignment out of line |
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

The delinked target object finds these elements from the other side, because the executable does not say which section a string came from. For an address in `.rodata` it reads the target's string both ways, as bytes up to a zero byte and, from a 4-aligned address, as four-byte characters up to a zero character, and names the element of our object with the same content. An address that is passed on takes the wide reading first, since an empty wide string (four zero bytes) is also the empty narrow string. The same four bytes could instead be a narrow empty string followed by three zero bytes at an aligned address, and the content cannot tell the two apart. The recovered units reference no such address (their only narrow empty string is the terminator of a string at an odd address), and one that did would show in `hv diff` as `.rodata.str4.4` against our `.rodata.str1.1`. An address that is loaded from takes a binary constant of the access width first, so a zero float stays in its `.rodata.cst4` element, and a wide string second, because the first character of a wide string is loaded through the string's own address. The element goes into the target's copy of our merged section at the same offset, so both sides reference the same section and objdiff compares the references. The target has a copy of every merged section of ours, zero-filled where no reference found a string, because objdiff's report combines the sections of a class (`.rodata.str1.1` and `.rodata.str1.8`, say) only when an object has at least two, and the two objects must combine alike. A literal that matches nothing keeps its widest read in `.rodata.lit`.

A label inside one of our functions, such as a jump table case or the label a variadic prologue loads before its computed jump, is named by that function's section and the offset the function has in the target object, shifted when a longer target function before it moves it. This is the reference the assembler wrote for ours, a section symbol and an addend, and a table entry that points into an unrecovered function keeps its `sub_` name.

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
