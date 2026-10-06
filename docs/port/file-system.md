# File system

Every file the game opens goes through `daisy::io::CFileSystem`
([`src/daisy/io/CFileSystem.cpp`](../../src/daisy/io/CFileSystem.cpp)). It adds three things to plain
stdio: directory aliases, paths that reach into zip archives, and file lists that work in folders and
archives alike. Zip-level rules are in [zip-archives.md](zip-archives.md).

## Aliases

- `addDirectoryAlias("$NAME$", dir)` stores the key with both dollar signs. Lookups are
  case-sensitive.
- `CHarvestFullMain` sets `$HARVEST_USERDATA$` from `IOSOperator::getApplicationSupportPath("Harvest")`
  (`~/.Harvest` on Linux). `$GAME_RESOURCES$` is set by the device.
- Resolving looks at the first `$` and the next `$` only. If that alias has a non-empty value, it
  drops everything before the first `$`, joins the value and the remainder without adding a
  separator, and resolves the result again, so aliases can chain.
- An unknown alias leaves the path unchanged (and inserts an empty entry for it).
  `getDirectoryFromAlias` returns `""` for unknown aliases.
- Otherwise paths go to libc as given: `/`-separated, relative to the process's working directory,
  case-sensitive.
- `resolveAliases` returns a `CFilePath` that the caller must drop.

## Paths into archives

- The registered archive extensions start as `{".zip"}`; `addZipAlias` adds more.
- `getZipInPath` checks the extensions in order. The first one found at a position above 0 cuts the
  path right after it: `a/b.zip/c` gives the archive `a/b.zip`. Matching is case-sensitive, so
  `.ZIP` is not an archive.
- Archive readers are cached by the exact string passed to `getZipReader`, which opens the
  alias-resolved path. They stay open until `dropZipReaders` or until the file system is destroyed.
- `createAndOpenFile(name)` tries the resolved path on disk first. Otherwise it splits the unresolved
  name with `getZipInPath` and opens the rest (after the archive name and its `/`) inside the archive.
- `existFile(name, searchArchives)` checks the disk with `fopen(resolved, "rb")`, so directories count
  as existing. With `searchArchives` it also consults archive readers that are already open under the
  resolved archive path (`createFileList` opens them); it never opens an archive itself.

## Working directory

- `changeWorkingDirectoryTo(dir)` first tries `chdir`. On success the working directory is the string
  as given and the file system is not inside an archive.
- Otherwise it splits `dir` with `getZipInPath` and opens that archive. Working directories inside an
  archive are virtual: the process's working directory does not change. The check that the folder
  exists inside the archive is broken (see [original-bugs.md](original-bugs.md)), so any path under a
  readable archive is accepted.

## File lists

`createFileList(filter, directory, mode)` saves the working directory, changes into the resolved
directory, builds the list and changes back.

- **In a folder** (`CFileList`): `glob(filter, GLOB_MARK)` in that folder. Patterns are shell-style and
  case-sensitive; dotfiles need a leading `.`. There is no recursion. The order is glob's `strcoll`
  sort, so it depends on the locale (in the "C" locale uppercase sorts first). Directory names keep a
  trailing `/`, which is also what marks them as directories. Modes: 0 lists everything, 1 files only,
  2 directories only. A full name is the unresolved directory, a `/` unless it already ends in one,
  and the name.
- **The port** replaces glob with its own (`port/src/platform/glob.cpp`, through `#ifdef HARVEST_PORT`
  in `CFileList.cpp`), since Windows has none: the same shell-style, case-sensitive match with the
  leading-dot rule, `/` after directory names (symbolic links followed), and **byte order** (`strcmp`)
  instead of `strcoll`. The original's order follows the user's locale (`CLinuxOperator`'s
  constructor calls `gtk_init`, which calls `setlocale(LC_ALL, "")`; under `en_US.UTF-8` case is
  mostly ignored); the port's is the "C" locale's, the same everywhere. Patterns containing `/` are not supported (the game passes none).
  On Windows the names come from `FindFirstFileW` as UTF-8, the encoding the C library's file
  functions take there once the port has set a UTF-8 locale (`port/src/platform/locale.h`); the
  pattern is matched by the port, so Windows' short 8.3 names never match.
- **Folders do not exist on Windows**: `existFile` opens the path with `fopen`, which fails for a
  folder on Windows, so the game's checks before creating its user data folders always create them
  again (the second `mkdir` fails harmlessly).
- **In an archive** (`CZipFileList`): see [zip-archives.md](zip-archives.md). The mode is ignored.
- If the directory can be entered neither on disk nor as an archive, the current working directory
  is listed instead. On a fresh install `$HARVEST_USERDATA$/mods/` does not exist, so the scan for
  user mods lists the game's working directory (it finds no `.hmd` or `.zip` files there in practice).

The game lists mods with `*.hmd` and `*.zip` in mode 1 in `$GAME_RESOURCES$/harvestClientData/mods/`,
then in `$HARVEST_USERDATA$/mods/`, then `*.hmd` in mode 0 inside each archive. Mod and profile order
follows these sort orders.

## Other operations

- `zipDeflateData` and `zipInflateData` make a zlib stream with header (`deflateInit` at the default
  level, plain `inflateInit`) in a single call; the written size is `total_out`. The first buffer
  argument is the output. Save games use them (see [save-games.md](save-games.md)).
- `createDirectory` is `mkdir(resolved, 0755)` (the port's Windows build calls `mkdir(resolved)`);
  `deleteFile` is `remove(resolved)`.
- `readFileIntoMemory` reads the whole file into a `CMemoryReadFile` named after the unresolved name.
- Read files open with `"rb"`, write files with `"wb"` or `"ab"`. `getModifiedDate` is `st_mtime`.
