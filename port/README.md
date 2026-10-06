# Harvest port

The modern port compiles the recovered C++ in [`../src`](../src) with `zig c++` and supplies its own
platform layer in [`src/`](src). The plan is in [`docs/port/porting-plan.md`](../docs/port/porting-plan.md);
what the recovered code still needs from the port is in [`docs/port/census.md`](../docs/port/census.md).

## Building

Needs zig 0.17.0 (`just zig` downloads the pinned one into `build/tools/zig`) and, the first time,
network access to fetch the dependencies (github.com, www.lua.org). From this directory:

    zig build                      # the libraries in zig-out/lib, for the host
    zig build -Doptimize=ReleaseFast
    zig build -Dtarget=x86_64-windows-gnu   # or x86_64-linux-gnu, aarch64-macos, ...
    zig build harvest              # the game executable, zig-out/bin/harvest
    zig build run                  # build and run it (arguments after --)
    zig build census               # links every kept unit with the original's plain loop
    zig build tests                # the test programs in tests/, without running them
    zig build test-audio -- ../orig/1.18-linux-amd64/harvestClientData/sfx/ [--wav out.wav] [--play]
    zig build test-menu_scene -- ../orig/1.18-linux-amd64 [shader level] [width height]
    zig build test-text -- ../orig/1.18-linux-amd64 [scratch dir]

`harvest` and `census` link every kept unit and port source, so a missing seam shows up as a linker
error: those errors are the census. For the grouped report with callers, from the repository root:

    uv run python port/tools/census.py            # the port's unresolved symbols, by owner
    uv run python port/tools/census.py --libc     # plus the C library calls the kept units make
    uv run python port/tools/census.py --original # the matching build's objects (run `just match` first)

## Running

From the repository root, `just` wraps the common commands (the game data defaults to
`orig/1.18-linux-amd64`):

    just run              # debug build with UBSan, then run; extra args go to the game (just run --no-audio)
    just play             # optimized build without UBSan stops, then run
    just build            # debug build only; just build -Doptimize=ReleaseFast -Dtarget=x86_64-windows-gnu
    just build-all        # macOS, Linux, Windows and web builds from this machine
    just serve-web        # the web build on http://127.0.0.1:8000 (needs Emscripten)
    just port-test audio  # also video, menu_scene, text
    just smoke-windows    # the Windows build under Wine, intro to a game; screenshots in build/smoke/windows
    just census           # unresolved symbols, if any
    just package          # release packages in zig-out/packages (just package macos|windows|linux)

Directly:

The game needs the original's data, the directory that holds `harvestClientData/`:

    zig-out/bin/harvest --data ../orig/1.18-linux-amd64   # or set HARVEST_DATA
    zig-out/bin/harvest --null-video --no-audio --no-vsync
    zig-out/bin/harvest --scale 1   # the game at the drawable's full resolution (default: the display's scale)

Without `--data` or `HARVEST_DATA` the game uses the folder given last time, else searches next to
the executable (or the app bundle), the Steam libraries (`steamapps/common/Harvest Massive
Encounter`, including the Mac release's `Harvest Steam.app`), `/Applications` on macOS, `~/Games`
on Linux and the user data folder, and when nothing is found it shows where it looked and exits.
The full order is in
[`docs/port/input-and-window.md`](../docs/port/input-and-window.md#game-data).

User data goes to `~/.Harvest` on Linux, `~/Library/Application Support/Harvest` on macOS and
`%APPDATA%\Harvest` on Windows. The device and its options are described in
[`docs/port/input-and-window.md`](../docs/port/input-and-window.md#the-ports-device).

## Linux smoke test

`just smoke-linux` ([`smoke/smoke-linux.sh`](smoke/smoke-linux.sh)) cross-builds the debug port and
its tests for Linux, builds a Debian container ([`smoke/Dockerfile`](smoke/Dockerfile): Xvfb, a
headless Weston, Mesa's llvmpipe, PulseAudio, PipeWire, ALSA; no game files) and runs the game in it
against the data mounted read-only. [`smoke/menu.script`](smoke/menu.script), an input script on a
fixed 60 Hz clock, walks from the intro through a new profile (its name partly pasted with Ctrl+V on
X11), settings, the creative-mode mod list and a normal game to saving it and loading it again,
taking a screenshot at each step; then the renderer test draws its scenes in both OpenGL profiles.
It fails when the game exits badly or reports a UBSan check, when the OpenGL context or audio
backend is not the one asked for, when `~/.Harvest` has no profile or the paste did not arrive, or
when a screenshot is missing or blank.

    just smoke-linux                                  # the Docker host's architecture, X11, PulseAudio, ES
    just smoke-linux --display wayland --audio pipewire --gl core
    just smoke-linux --arch x86_64                    # under emulation
    just smoke-linux --matrix                         # X11 and Wayland, every sound path, both profiles, both architectures
    just smoke-linux --original                       # also: the port on the original build's ~/.Harvest

Each configuration's report, log, screenshots, `~/.Harvest` and renderer-test frames land in
`build/smoke/linux/<arch>-<display>-<audio>-<gl>/`. `--gl core` makes Mesa refuse OpenGL ES 3.0
(`MESA_GLES_VERSION_OVERRIDE=2.0`), so the device falls back to OpenGL 3.3 core. Loose mods with
names that sort differently in byte order and in a locale are put in `~/.Harvest/mods` first, so
`09-mods.jpg` shows the order.

`--original` also builds an x86-64 image with the original 1.18 Linux build's libraries
([`smoke/original/Dockerfile`](smoke/original/Dockerfile): NVIDIA Cg from Debian non-free, GTK 2,
OpenAL) and plays the original from `orig/1.18-linux-amd64` with xdotool on the wall clock: a new
profile, a normal game, a save (`build/smoke/linux/original/`, with its screenshots). The port then
runs on that `~/.Harvest` with [`smoke/original.script`](smoke/original.script), picks the profile and
loads the save (`<configuration>-original/`).

## What is compiled

- **Kept units:** every unit in [`config/1.18-linux-amd64/units.toml`](../config/1.18-linux-amd64/units.toml)
  except the platform units listed in `replaced` in [`build.zig`](build.zig): the Linux device and
  OS operator, the SFML joystick driver, the OpenAL backend, the OpenGL driver and its material
  renderers, the Cg material renderer base, the software z-buffer, and `HarvestFull/main.cpp`. Newly
  recovered units join the build automatically.
- **Port sources:** every `.cpp` under [`src/`](src) except `src/main.cpp`, which only the `harvest`
  executable links (the census uses [`census/main.cpp`](census/main.cpp) instead). New files join the
  build automatically, so platform work needs no `build.zig` edits.
- **Test programs:** every `.cpp` directly in [`tests/`](tests) is an executable linked against the
  library (only the objects it uses), run with `zig build test-<name> -- args`.
- **Dependencies** ([`build.zig.zon`](build.zig.zon), pinned by hash, built from source for the
  target, installed to `zig-out/lib` and `zig-out/include`):

  | Library | Version | Built as | Used for |
  |---|---|---|---|
  | zlib | 1.3.2 | `libz`: the core (no `gz*` file API) | save-game streams, zip archives |
  | PUC Lua | 5.1.5 | `liblua`: core and standard libraries, `LUA_USE_POSIX` except on Windows; headers include `lua.hpp` | scenarios, entity scripts |
  | stb_image | nothings/stb `2c980bb` | `libstb_image`: JPEG, TGA and PNG from memory ([`src/thirdparty/stb_image.c`](src/thirdparty/stb_image.c)) | texture loading |
  | miniaudio | 0.11.25 | `libminiaudio` with stb_vorbis ([`src/thirdparty/miniaudio.c`](src/thirdparty/miniaudio.c)); include `<miniaudio.h>` | the audio backend |
  | SDL3 | 3.4.16 | `libSDL3`, static, from [castholm/SDL](https://github.com/castholm/SDL) 0.5.4 (SDL ported to the zig build system) | window, events, input, timing |

  Every game module links all five, so any port source can include their headers. UBSan stays on
  for the game code in Debug builds and is off for the third-party C.

Flags: `-std=gnu++98` (GCC 4.4's default dialect, which the recovered code needs no changes for) and
`-DHARVEST_PORT`, which guards the port-only lines in shared sources. Port sources use the same
flags. Debug builds keep zig's UBSan (trap mode); nothing in the kept units trips it at compile time.

## Targets

From macOS: the host, `aarch64-macos`, `x86_64-macos`, `x86_64-linux-gnu` and `x86_64-windows-gnu`
build and link. SDL3 needs Xcode (or the Command Line Tools) for macOS targets; for an explicit
macOS `-Dtarget`, which zig does not treat as native, `build.zig` takes the SDK from `xcrun`.
Linux targets fetch castholm's `SDL_linux_deps` package on first use. Windows release builds use
the GUI subsystem (debug builds keep the console) and carry an icon and version resource.
`wasm32-emscripten` builds the web page, `harvest.js` and `harvest.wasm` with Emscripten's sysroot
and linker on `PATH` ([`docs/port/web.md`](../docs/port/web.md)).

## Packages

`just package` builds a universal, ad-hoc signed macOS app, a Windows zip and a Linux tarball into
`zig-out/packages`, with the port's binary, a README and the libraries' licences, never game data.
See [`docs/port/packaging.md`](../docs/port/packaging.md), and [`packaging/`](packaging) for the
Info.plist, resource script, desktop entry and icon.

## Source rules

`src/` at the repository root is shared with the matching build and must stay byte-identical under
GCC 4.4 (`just match`). Port-only code in a shared file goes inside `#ifdef HARVEST_PORT`; prefer
adding a file under `port/src/` over patching a shared one. Headers under `port/src/` are included
by their path from there (`"platform/glob.h"`, `"video/ColorFormats.h"`).
