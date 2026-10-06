# Original bugs

The 1.18 executables contain a few behaviours that read as genuine bugs once the recovered source is
traced to its effect. The recovered source keeps all of them, because it has to compile to the same
code; each is marked with a comment at the place it happens. This page collects them for a port: what
the original does, what it affects, and what a port should do.

Entries are confirmed in the Linux amd64 build, and in the Mac build where noted. Add new findings
here with the address and the recovered file.

## Bugs with visible effects

### 1) Changing into an archive folder other than the first one hangs

Native behaviour:

- `CZipReader::directoryExists` (`0x539f30`, [`CZipReader.cpp`](../../src/daisy/io/CZipReader.cpp))
  returns true for `""`. For any other folder it compares only the first entry's path with the
  argument, because its loop never advances the iterator.
- So it returns true when the first sorted entry is that folder, and loops forever otherwise.
  `CFileSystem::changeWorkingDirectoryTo` calls it on Linux.

Impact:

- None with the shipped mods: each has exactly one folder entry (such as `rush/`), which sorts first.
  A mod archive with two top-level folders, or with a top-level file that sorts before its folder,
  would hang the game while listing mods.

Port recommendation:

- Implement the intended rule: a folder exists when some entry's path starts with `dir/`.

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
