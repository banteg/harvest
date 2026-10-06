# Zip archives

Mods ship as zip archives. The game reads them with `daisy::io::CZipReader` and lists them with
`daisy::io::CZipFileList` ([`src/daisy/io/`](../../src/daisy/io/)), both derived from Irrlicht 0.7's zip
reader. A port can use any zip library, but it has to reproduce the lookup and listing rules below,
which differ from a normal zip library.

## Reading an archive

- Only local file headers are read, from offset 0 until a record without the signature `PK\3\4`. The
  central directory is never read. There is no zip64 or encryption support. Header fields are read
  one at a time as little-endian.
- Every `\` in a stored name becomes `/`. The entry's name is the part after the last `/` (a folder
  entry such as `dir/` has an empty name). Its path is the whole stored name, folders included
  (`dir/file.lua`), or empty when the name has no folder. Irrlicht kept only the folder here; daisy
  keeps the whole name.
- Entries with a data descriptor (flag bit 3) are found by scanning byte by byte for `PK\7\8`, then
  reading the CRC, compressed size and uncompressed size. Other entries are skipped by their
  compressed size.
- After the scan, entries are sorted by name (signed chars, shorter first on a tie), so folder entries
  come first.

## Opening an entry

- Stored entries (method 0) open as a window over the archive file (`CLimitReadFile`), seeked to the
  data and sized by the uncompressed size.
- Deflated entries (method 8) are read whole and inflated into memory as raw deflate
  (`inflateInit2_` with window bits -15), then served as a `CMemoryReadFile`. The result of `inflate`
  is ignored, so a truncated stream still opens.
- Any other method logs "file has unsupported compression method." and opens nothing.

## Lookup

`CFileSystem::getZipReader` creates readers with case-sensitive, full-path lookup. A lookup:

1. strips one leading `/`;
2. splits the request at its last `/`: the name is the part after it, the path is the whole request
   (or empty without a `/`);
3. searches linearly for an entry whose name and path both equal the request's, case-sensitively.

So `rush/main.lua` finds the entry stored as `rush/main.lua`, and a bare `x` finds only a top-level `x`.

## Changing into a folder of an archive

`directoryExists(dir)` returns true for `""`. For any other folder it compares only the first entry's
path with the argument, because its loop never advances the iterator: it would return true when the
first sorted entry matches and loop forever otherwise. In the shipped game it is only ever called with
`""` (see the file system bug in [original-bugs.md](original-bugs.md)), so the hang cannot happen. A
port should implement the intended rule: a folder exists when some entry's path starts with `dir/`.

## Listing a folder of an archive

`createFileList` lists an archive folder when the working directory is inside a zip. It passes the
filter and the working directory with the zip prefix removed (`""` for the archive root, otherwise for
example `rush/`):

- An entry is listed when its folder part (path minus name) equals that directory exactly,
  case-sensitively. Entries in subfolders are not listed. A folder's own entry appears inside that
  folder with an empty name.
- A filter that starts with `*` and is at least two characters long keeps names ending in the rest
  (`*.lua` keeps `.lua` files). Any other filter, including one with `*` elsewhere, lists everything.
- Entries come out in the sorted order above. The name is the entry's name, the full name is the
  stored path, the size is the uncompressed size, and `isDirectory` is always false. The `mode`
  argument is ignored for archives.
- The getters accept `index == count` and read one entry past the end; a negative index returns 0 or
  false.
