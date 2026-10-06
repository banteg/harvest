# Original bugs

The 1.18 executables contain a few behaviours that read as genuine bugs once the recovered source is
traced to its effect. The recovered source keeps all of them, because it has to compile to the same
code; each is marked with a comment at the place it happens. This page collects them for a port: what
the original does, what it affects, and what a port should do.

Entries are confirmed in the Linux amd64 build, and in the Mac build where noted. Add new findings
here with the address and the recovered file.

## Bugs with visible effects

### 1) Any path under a readable archive is accepted as a working directory

Native behaviour:

- `CFileSystem::changeWorkingDirectoryTo` (`0x530540`, [`CFileSystem.cpp`](../../src/daisy/io/CFileSystem.cpp))
  falls back to archives when `chdir` fails: it splits the path at the archive and asks the archive
  whether the folder inside it exists. It takes that folder from the archive path itself instead of
  from the requested directory, so `CZipReader::directoryExists` always receives `""` and always says
  yes.
- `directoryExists` (`0x539f30`, [`CZipReader.cpp`](../../src/daisy/io/CZipReader.cpp)) is broken as
  well: for a non-empty folder its loop never advances and would hang. The bug above means it is never
  reached with one.

Impact:

- Changing into a folder that does not exist inside a mod archive succeeds, and the following list
  is simply empty. The shipped mods never ask for one, so nothing visible happens.

Port recommendation:

- Check the real folder inside the archive: it exists when some entry's path starts with `dir/`.

### 2) A dropped highscore connection is never reported

Native behaviour:

- When the connection closes before a response, `CHTTPConnectionHandler`'s network callback
  ([`CHTTPConnectionHandler.cpp`](../../src/ox/net/CHTTPConnectionHandler.cpp)) sends an
  `ENET_HTTP_ERROR` event with the data `"Interrupted"`, but never sets the event's `EventType`. Every
  other event it sends sets `EventType = EET_NETWORK_EVENT`. Both builds do this.
- The receivers, `CHighscoreScreen::OnEvent` and `CStatisticsScreen::OnEvent`, dispatch on
  `EventType == EET_NETWORK_EVENT` first, so the error is dropped unless the uninitialised stack value
  happens to match.

Impact:

- If the server drops the connection mid-request, the highscore screen or the statistics upload stays
  in its waiting state instead of showing the error.

Port recommendation:

- Set `EventType = EET_NETWORK_EVENT` on the interrupted event. (The original server is gone, so a
  port that keeps online highscores will use its own backend anyway.)

### 3) Entities spawned during an update skip their first update and render

Native behaviour:

- `COxEntityManager::updateAllEntities`
  ([`COxEntityManager.cpp`](../../src/ox/entity/COxEntityManager.cpp)) moves the entities created
  during the update (the pending list) into the layer, then has a second loop meant to give each a
  tiny update (`update(0.0001f)`) and add it to this frame's render list.
- That second loop reuses the first loop's iterator, which is already at the end of the pending list,
  so it never runs. Both builds do this.

Impact:

- Every entity spawned during an update (projectiles, particles, spawned aliens, buildings placed by
  scripts) appears one frame later than intended, and is not given its initial tiny update.

Port recommendation:

- Keep the one-frame delay by default, since all gameplay timing was tuned with it. Offer the
  intended behaviour (reset the iterator before the second loop) only as an option.

### 4) A pending charge-bomb explosion breaks the rest of the save

Native behaviour:

- `CEntityManager::writeEntities` saves layers 0 to 3, which includes the perimeter bomb's explosion
  (type 12, layer 3, alive for 3 s). `CPerimeterBombExplosion::writeEntityData` writes three floats.
- `CEntity::readNextEntity` (`0x432a60`, [`CHarvestEntity.cpp`](../../src/HarvestFull/harvest/entity/CHarvestEntity.cpp))
  has no case for type 12, so it creates nothing and leaves those three floats unread.

Impact:

- A game saved during the three seconds of a charge-bomb explosion is misread from that record on:
  every following entity and the rest of the file are read 12 bytes out of step.

Port recommendation:

- Read the type 12 record and restore the explosion (or at least skip its three floats). Saves made by
  the original in that window are already damaged; a port can detect them by the misread.

### 5) A mod section with no mods shifts the rest of the load

Native behaviour:

- `CLuaManager::writeLuaStates` always writes a mod count followed by a value count.
  `CLuaManager::initLuaBySaveFile` (`0x44fe70`, [`CLuaManager.cpp`](../../src/HarvestFull/harvest/game/CLuaManager.cpp))
  returns as soon as it reads a mod count of 0, without reading the value count.

Impact:

- A save from a creative game with mods available but none ticked (they start unticked) would load
  4 bytes out of step after the Lua section. Found in the code, not reproduced in the game.

Port recommendation:

- Read the value count even when the mod count is 0.

### 6) A save that does not compress is silently truncated

Native behaviour:

- `CFileSystem::zipDeflateData` and `zipInflateData` (`0x52c760`, `0x52c6c0`) make a single
  `deflate`/`inflate` call with `Z_FINISH` and accept `Z_OK` as success. The deflate output buffer is
  only as large as the input.

Impact:

- A payload that does not shrink would be written truncated with no error. Game states compress well,
  so this is latent.

Port recommendation:

- Size the output with `deflateBound` and require `Z_STREAM_END`. (The original also skips
  `deflateEnd`/`inflateEnd` on its error paths, leaking the zlib stream.)

### 7) Every 2D line is 1 px wide

Native behaviour:

- `CVideoOpenGL::draw2DLine` and `draw2DLineFloat` (about `0x4ed940` and `0x4ed7e0`,
  [`CVideoOpenGL.cpp`](../../src/daisy/video/OpenGL/CVideoOpenGL.cpp)) call `glLineWidth(3.0)`
  between `glBegin` and `glEnd`, where OpenGL rejects it with `GL_INVALID_OPERATION`.

Impact:

- Every 2D line the game draws (selection boxes, the minimap frame, the shuttle race's split line,
  lines drawn by Lua scripts) is 1 px wide, not the intended 3 px.

Port recommendation:

- Draw 1 px lines: that is what players saw.

## Bugs without visible effects

- **`CFileSystem::renameFile`** (`0x52f3e0`) calls `rename(newName, filename)`, the reverse of its
  parameters. Nothing in the game calls it; a port should use the obvious order.
- **Archive readers opened twice**: `createAndOpenFile` caches archive readers by the unresolved path,
  `createFileList` and `existFile` by the resolved one, so each mod archive ends up open twice. It
  costs a file handle and memory. A port should key the cache by the resolved path.

- **Driver null checks** ([`CVideoNull.cpp`](../../src/daisy/video/Null/CVideoNull.cpp)), all on paths
  the game never takes: `getTexture(IReadFile*)` (`0x4e4940`) dereferences a null file while logging;
  `setInputTexture` (`0x4e1ad0`) grabs a null texture when there are no post-processing surfaces;
  `getDynamicLight` (`0x4dfa50`) returns a null reference for an out-of-range index; and
  `makeColorKeyTexture(pos)` (`0x4e07b0`, inherited from Irrlicht) reads the key pixel at
  `y * width` instead of using the pitch.

- **Renderer, latent or harmless** ([`src/daisy/video/OpenGL/`](../../src/daisy/video/OpenGL/)):
  `setScissorRect` (`0x4ec430`) computes y as `LRY − height` instead of `height − LRY` (the only caller
  passes full-height rectangles, so y is 0); `setBasicRenderStates` (`0x4ec4c0`, from Irrlicht 0.7)
  reads colours, shininess, the filter and lighting from the driver's current material instead of
  its argument; `addDynamicLight` limits lights by the enum value `GL_MAX_LIGHTS` instead of the
  queried count; `loadExtensions` leaves extension pointers uninitialised without
  `ARB_multitexture`, opens an X display it never closes, compares GLU version characters with the
  numbers 1 and 2, and loads `glUniform4fvARB` twice; the ARB shader renderer binds the fragment
  program with the vertex program's name; the GLSL renderer binds texture stages 2 and 3 of a
  two-texture material and uses a uniform's list index as its location; `COpenGLTexture::copyTexture`
  (`0x5aaa70`) copies A8R8G8B8 data as one block of the padded size, so a non-power-of-two image
  would be read past its end; and the Cg renderer never drops its callback. None of these affects the
  shipped game.

## Linux device bugs

These are in the Linux-only device code ([`src/daisy/other/`](../../src/daisy/other/)). A port replaces
this layer, so it only needs to avoid them; the key-code ones also explain odd key bindings in Linux
profiles.

- **Unmapped keys report garbage.** The key table (built in the `CIrrDeviceLinux` constructor,
  `0x4bbc40`) never initialises the entries for Insert, Pause, Control, Alt, System, RShift, Menu,
  brackets, semicolon, quote, slash, backslash, tilde and equals, so those keys report whatever was in
  the heap (usually 0) and cannot be bound reliably. `postKeyEvent` (`0x4baff0`) also indexes the table
  without a bounds check, so SFML's `Unknown` key (−1) reads the neighbouring field. A port should map
  unknown keys to 0.
- **Ctrl with another key sends an undefined event.** In `postKeyEvent`, Ctrl plus a key other than
  V, F or Q leaves the key event type uninitialised, so handlers see a stack-garbage type. It should be
  a normal key press.
- **Joystick button releases also report a connection.** In `CIrrDeviceLinux::run` (`0x4bb6a0`) the
  button-released case falls through into the joystick-connected case, so every release also posts a
  "connected" event (type 3).
- **Ctrl+Q frees the window twice.** `closeDevice` (`0x4badd0`) deletes the SFML window without
  clearing the pointer; `run()` then polls the freed window and the destructor deletes it again.
- **The fullscreen flag starts uninitialised.** If it happens to read as windowed, the first
  `setFullscreenMode(false)` returns early and the render size stays 0×0 until a resize event, so the
  relative cursor position divides by zero.
- **Formatted engine logs print garbage.** `os::Printer::log` and `logWithInfo` (`0x4c1ea0`–`0x4c2060`)
  pass their `va_list` as the first variadic argument, so the first `%` argument of every formatted
  log line is garbage. `CLogger::logWithInfo` reuses a `va_list`, and messages over 128 characters go
  through a buffer that is freed before use.
- **No error dialogs on Linux.** `CLinuxOperator::messageBox` and the other value-returning OS stubs
  (OS version, computer name, registry, process id and name, `runApplication`) have empty bodies, so
  `CGameMain`'s missing-file error box shows nothing and the others return garbage. `openURL` passes
  the URL to `system("xdg-open …")` without quoting.
- Smaller ones: `swapBuffers` is declared to return `bool` but returns nothing (no caller uses it);
  the `/proc/self/exe` path is read without terminating or checking it; `getApplicationSupportPath`
  gives `/.Harvest` when `HOME` is unset; and the video mode list's getters accept `index == count`.

## Mod API quirks to keep

Mods depend on these, so a port should keep them and document them for modders (see
[mods-and-files.md](mods-and-files.md)).

- **`harvest.isPositionBlocked`** with a building id (`0x445740`) returns true when the building fits.
- **`harvest.findAliens`** compares the type with the internal 0-based value, while `spawnAlien` and
  `getAlienType` use 1-based types.
- **Hooks** that have no handler the first time they fire are never called again in that game.

## Audio quirks

- **`CAudioDriver::loopSound`** (`0x5b3830`) ducks by 0.25 even when voice ducking is turned off.
- **Oriented sounds**: `COpenALDriver::devicePlayOrientedSound` pans +0.5 when the source is to the left,
  while `playParticleSound` passes positive for the right, so particle sounds appear to pan to the
  opposite side (inferred from the code, not checked by ear).

## Layout quirks kept for parity

These are visible but small, and the game's art was made against them, so a port should keep them.

- **Window frame edges** (`CGUIWindow::updateAnimationRects`,
  [`CGUIWindow.cpp`](../../src/daisy/gui/CGUIWindow.cpp)): the bottom edge starts at the bottom-right
  corner's width (not the bottom-left's), and the right edge is as wide as the left edge. Both builds.
- **Statistics screen** (the element-drawn handler in `CStatisticsScreen::OnEvent`,
  [`CStatisticsScreen.cpp`](../../src/HarvestFull/harvest/gui/CStatisticsScreen.cpp)): after the
  overall background it advances by the diagram background's height instead of its own.
- **Layout rows** (`IGUILayout::sortRiver`,
  [`IGUILayoutInline.h`](../../src/ox/gui/IGUILayoutInline.h)): the first row's vertical alignment
  starts from the horizontal alignment value. Both are 0 in every use, so it has no effect today.

## Undefined behaviour without visible effects

Out-of-bounds or mismatched operations that happen to be harmless in the shipped game. A port should
simply write the intended code.

- **`CGUITabButtonRow` constructor** ([`CGUITabControl.cpp`](../../src/daisy/gui/CGUITabControl.cpp))
  clears 10 animation pointers (`ETCA_COUNT`, the tab control's count) into a 6-entry array
  (`ETBRA_COUNT`), zeroing the first two `Rects` entries, which are set later.
- **`CHighscoreScreen` constructor**
  ([`CHighscoreScreen.cpp`](../../src/HarvestFull/harvest/gui/CHighscoreScreen.cpp)) clears
  `PromoteButtons[4][2][4]` as if it were `[4][4][4]`; the flat index reaches 39 of 32, zeroing the
  seven member pointers after the array (all set later) and the first word of an already initialised
  mutex.
- **`ansiToWide`** ([`CStringConversions.h`](../../src/ox/core/CStringConversions.h)) frees its
  temporary `wchar_t` array with scalar `delete` instead of `delete[]`.
- **Zip file listings** ([`CZipFileList.cpp`](../../src/daisy/io/CZipFileList.cpp)): the getters accept
  `index == count` and read one entry past the end.
- **Zip reading** ([`CZipReader.cpp`](../../src/daisy/io/CZipReader.cpp)): the result of `inflate` is
  ignored, so a truncated deflated entry still opens with partial data; and the data-descriptor search
  does not retry a mismatched byte as the start of the signature (a `PK` immediately followed by
  `PK\7\8` is missed). Neither occurs with the shipped archives.
- **Image loaders** (see [textures.md](textures.md)): the JPEG loader assumes three components, so a
  grayscale JPEG would decode garbled, and the TGA RLE decoder has no bounds check. No shipped file
  triggers either.
