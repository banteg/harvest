# Porting plan

The port keeps the recovered C++ (game, engine and GUI toolkit) and replaces only the platform seams
underneath it. It targets macOS, Linux, Windows and the web.

## Decisions

- **Keep the C++.** `src/` is shared by the matching build and the port. The port does not rewrite
  game logic; it supplies its own implementations of the platform interfaces and patches shared
  source only where code is tied to a platform.
- **Shared source stays matching.** Every edit to `src/` must leave the GCC 4.4 build byte-identical
  (`just match` is the guard). Port-only code in shared files goes inside `#ifdef HARVEST_PORT`, which
  the matching build never defines; prefer replacing a whole platform unit in `port/` over patching it.
- **Compiler: `zig c++`** (clang with libc++) from zig 0.17.0 (`just zig` downloads the pinned build into
  `build/tools/zig`), driven by `port/build.zig`. Dependencies are pinned in
  `build.zig.zon` and built from source, so every target cross-compiles from one machine.
- **Libraries:**
  - SDL3 for the window, events, text input, gamepads, clipboard, video modes, paths, directory
    listing and timing;
  - our own renderer over OpenGL ES 3.0 / WebGL 2 (OpenGL 3.3 core on desktop), implementing the
    daisy video driver interfaces with the rules in [renderer.md](renderer.md);
  - miniaudio for mixing, streaming and Ogg Vorbis, behind the audio driver interface;
  - PUC Lua 5.1.5 instead of LuaJIT;
  - stb_image for JPEG and TGA (with the original's always-flip TGA rule), and zlib or miniz;
  - the menu's Cg shaders translated to GLSL ES 3.0.
- **Dropped:** Cg, GTK, SFML, OpenAL and ALUT, X11, LuaJIT, the online highscores (the server is
  gone; the network device fails cleanly).
- **Web:** Emscripten through zig: zig compiles against Emscripten's sysroot and em++ links
  ([web.md](web.md)). The main loop must not block, so the game's frame step runs from SDL3's main
  callbacks on every platform.
- **Original bugs** ([original-bugs.md](original-bugs.md)) are fixed in the port by default where they
  break things, with a switch to keep the original behaviour (as Crimsonland's rewrite does with
  `--preserve-bugs`). Quirks the game was tuned around stay as they are.

## Known risks

- **clang and libc++ instead of GCC 4.4 and libstdc++.** Stricter template rules need source fixes
  that must stay codegen-neutral for the matching build. `std::sort` orders equal elements
  differently, which can change draw order; the port vendors libstdc++'s sort where order is visible.
- **UBSan** is on in zig's debug builds and will trap on the original's undefined behaviour (see the
  ledger); fix or exclude those spots.
- **`wchar_t` is 2 bytes on Windows.** File formats are safe (wide strings are stored as 16-bit
  units). Wide `printf` formats and the multibyte conversions are handled (mingw-w64's C99 `printf`
  and a UTF-8 locale, see [input-and-window.md](input-and-window.md#paths-and-os-services)) and
  checked by `port/tests/text.cpp`, which `just smoke-windows` runs under Wine.

## Milestones

1. **Scaffold and census.** `port/build.zig` compiles every kept unit with zig for the host; the
   compile fixes are in; a link census lists every unresolved symbol by owner.
2. **Seams.** SDL3 device, GL renderer, miniaudio backend, directory listing, Lua 5.1, the menu scene
   subset, stubs for networking. Goal: a native build that reaches the main menu and starts a game.
3. **Parity.** Original save games load; original-bug switches work; screenshots compare against the
   original on Linux.
4. **Platforms.** macOS, Linux and Windows builds from one machine, then the web build.
