# The web build

`just web` builds the port for the browser into `port/zig-out/web/`: the page
([`port/web/index.html`](../../port/web/index.html) and its assets), `harvest.js` and `harvest.wasm`.
`just serve-web` serves it on `http://127.0.0.1:8000`. Every push to master that touches the port
publishes it to [Cloudflare Pages](https://harvest.banteg.xyz/) through its Git integration.
GitHub Actions builds pull requests ([`.github/workflows/web.yml`](../../.github/workflows/web.yml)).
With nothing stored, the page downloads the data zip and starts the game.

Cloudflare runs `bash port/tools/build-web.sh` from the repository root and serves `port/zig-out/web`.
The script installs the same Zig 0.17.0 and Emscripten 6.0.10 as CI. Preview deployments are disabled.
[`wrangler.jsonc`](../../wrangler.jsonc) binds the existing `reflexive` R2 bucket as `REFLEXIVE`.
The page's Worker reads `harvest/harvestClientData.zip` at `/harvestClientData.zip`; all other routes
serve static assets without invoking the Worker. No bucket CORS configuration is needed.

## Building

The web is one more zig target, `-Dtarget=wasm32-emscripten`, with the same unit list, flags and
pinned libraries as the desktop builds. zig compiles everything; Emscripten (`em-config` and `em++`
on `PATH`, 6.0.10 in CI) only provides the sysroot and links.

- zig has no libc for Emscripten, so `build.zig` asks `em-config CACHE` for the sysroot (as it asks
  `xcrun` for the macOS SDK) and adds its headers. The C++ modules use Emscripten's libc++ instead of
  zig's (`link_libcpp` is off), and its headers go in with `-I`: libc++ must come before the
  compiler's own headers, which zig puts first among the system paths.
- SDL3 is the same package as on the desktop, given the sysroot's headers. It is built without
  threads.
- Lua 5.1 raises errors with `setjmp`/`longjmp`, which on the web need Emscripten's lowering
  (`-mllvm -enable-emscripten-sjlj`); em++ links its runtime. miniaudio is compiled as `gnu99`
  because its Web Audio backend uses `EM_ASM`.
- The `harvest` step runs `em++` on `src/main.cpp`'s archive (whole, so the linker takes `main` from
  it), the game library and the libraries, with the settings in `emscripten_link_flags`: WebGL 2,
  GL entry points through `SDL_GL_GetProcAddress` as the renderer loads them, a growing heap, the
  file system with IDBFS, and no automatic `main` (the page calls `callMain` once the data is in).
  Debug builds link Emscripten's UBSan runtime for the game code's checks.
- The first link after installing Emscripten builds its system libraries into its cache and prints
  that on stderr, which zig shows under the step; the build still succeeds.

## The page

- The player drops their install onto the disc (or picks the folder). Any folder that contains
  `harvestClientData` works: the Windows, Mac and Linux releases ship the same 168 data files byte for
  byte (the Mac app adds two stray copies), and a dropped `.app` is walked like a folder. A zip of
  any of these works the same way (Finder's Compress included): the page reads the central directory
  itself and inflates entries with the browser's `DecompressionStream`, skipping `__MACOSX`; zips
  inside `harvestClientData` are the game's mods and stay zipped. Only `harvestClientData` is copied.
- The data and the user data folder (`/libsdl/Harvest`: settings, profiles, saves) live in IndexedDB
  through IDBFS. The data is kept for the next visit (the page asks the browser not to evict it), and
  the user data is written back every 3 seconds and when the page is hidden. Eject forgets the data.
- With nothing stored, the page fetches `/harvestClientData.zip` from the same origin. The Worker
  streams the same object published at `https://reflexive.banteg.xyz/harvest/harvestClientData.zip`.
  Download progress is shown before the archive is extracted and cached. `?data=<url>` still
  loads a data folder from `<url>/manifest.json` (or a zip, for a `<url>` ending in `.zip`) for
  development, only when the page is served from localhost, so a link to the published page cannot
  hand players someone else's copy.
- `just serve-web` serves static files only. Use `?data=<url>` with a local zip or data manifest for
  local play, or use `wrangler pages dev port/zig-out/web` with a local R2 object to exercise the Worker.
- The game's fatal messages go to the page through `port::showErrorMessage`
  ([`Message.h`](../../port/src/platform/Message.h)) instead of a native dialog. On the web the
  window fills the page and Ctrl+Q does nothing.

## Audio

miniaudio uses its default Web Audio backend, a `ScriptProcessorNode`: deprecated, but in every
browser, and it mixes on the page's main thread, so a long frame can crackle. Its `AudioWorklet`
backend (`MA_ENABLE_AUDIO_WORKLETS`) would move the mixing off that thread, at a cost:

- links with `-sAUDIO_WORKLET -sWASM_WORKERS -sASYNCIFY` (a bigger, slower module);
- shared memory, so everything is compiled with atomics, and the page must be cross-origin isolated
  (`Cross-Origin-Opener-Policy` and `Cross-Origin-Embedder-Policy` headers). Cloudflare Pages can send
  these through an `_headers` file if this backend is enabled.

The mixer already locks around the audio callback (it runs on its own thread on the desktop), so the
game side is ready for it.
