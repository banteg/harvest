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

`harvest` and `census` link every kept unit and port source, so they fail until the platform seams
exist: their linker errors are the census. For the grouped report with callers, from the repository
root:

    uv run python port/tools/census.py            # the port's unresolved symbols, by owner
    uv run python port/tools/census.py --libc     # plus the C library calls the kept units make
    uv run python port/tools/census.py --original # the matching build's objects (run `just match` first)

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

From macOS: the host, `x86_64-linux-gnu` and `x86_64-windows-gnu` build and link (once the seams are
stubbed). SDL3 needs Xcode for macOS targets; for a non-native macOS architecture pass the SDK paths
as [castholm/SDL's README](https://github.com/castholm/SDL#macos) describes. Linux targets fetch
castholm's `SDL_linux_deps` package on first use. The web build comes later (Emscripten).

## Source rules

`src/` at the repository root is shared with the matching build and must stay byte-identical under
GCC 4.4 (`just match`). Port-only code in a shared file goes inside `#ifdef HARVEST_PORT`; prefer
adding a file under `port/src/` over patching a shared one. Headers under `port/src/` are included
by their path from there (`"platform/glob.h"`, `"video/ColorFormats.h"`).
