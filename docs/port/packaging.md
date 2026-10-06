# Packaging

`just package` (or `bash port/tools/package.sh [macos|windows|linux|all]`) builds release packages
of the port into `port/zig-out/packages/`, named `harvest-port-<version>-<platform>`; the version is
`build.zig.zon`'s. `all` builds every package the machine can: the macOS one only on macOS, the
Windows and Linux ones anywhere zig runs. Each package holds the port's executable, a player's
[`README.txt`](../../port/packaging/README.txt) and the licences of the linked libraries
(`zig build licenses` installs them into `share/licenses`). **Never game data**: players bring
`harvestClientData` from their own copy, and the game finds it as
[input-and-window.md](input-and-window.md#game-data) describes.

The builds are `ReleaseFast` and stripped (`-Dstrip=true`).

| Package | Contents | Runs on |
|---|---|---|
| `…-macos.zip` | `Harvest Port.app` (universal arm64 + x86_64, ad-hoc signed), README, licences | macOS 11 or later |
| `…-windows-x86_64.zip` | `harvest.exe` (GUI subsystem, icon and version resource), README, licences | Windows 10 or later (64-bit; needs the Universal CRT, which older Windows gets from an update) |
| `…-linux-x86_64.tar.gz` | `harvest`, `harvest-port.desktop`, `harvest-port.png`, `install-desktop-entry.sh`, README, licences | x86_64 with glibc 2.31 or later (Ubuntu 20.04, Debian 11); needs the system's OpenGL (or GLES) and X11 or Wayland libraries, which SDL loads at run time |

## macOS

- Both architectures are built with `-Dtarget=<arch>-macos.11.0` and joined with `lipo`. zig only
  finds the macOS SDK by itself for the host target, so for any explicit macOS target `build.zig`
  asks `xcrun --show-sdk-path` and hands the SDK's headers, frameworks and libraries to SDL and to
  the port's own modules. Xcode or the Command Line Tools must be installed.
- The bundle is `Harvest Port.app`, not `Harvest.app`: that is the original game's name, and copying
  the port into `/Applications` must not replace the original (and the data inside it). The menu
  bar still says Harvest.
- [`Info.plist`](../../port/packaging/macos/Info.plist): bundle id `io.github.banteg.harvest`
  (`HARVEST_BUNDLE_ID` overrides it), the version, `LSMinimumSystemVersion` 11.0,
  `NSHighResolutionCapable` (the game renders at the display's full resolution, see
  [renderer.md](renderer.md#in-the-port)), `NSSupportsAutomaticGraphicsSwitching` (OpenGL stays on
  the integrated GPU) and the strategy-games category.
- The bundle is signed ad hoc (`codesign --sign - --options runtime`) and verified with
  `codesign --verify --strict`. Ad-hoc signing lets it run on Apple silicon, but Gatekeeper still
  blocks a downloaded copy: players right-click the app and choose Open (or run
  `xattr -dr com.apple.quarantine "Harvest Port.app"`).
- The zip is made with `ditto -c -k --keepParent`, which keeps the bundle's signature valid.
- The data goes **next to** the app (or anywhere the search looks), not into its `Contents/Resources`:
  adding files to a signed bundle breaks its seal.

### Signing and notarizing for real

Not automated, since it needs an Apple Developer ID. With one:

1. `HARVEST_CODESIGN_IDENTITY="Developer ID Application: Name (TEAMID)" just package macos` signs with
   the hardened runtime (`--options runtime`, already used) and a secure timestamp (`--timestamp`,
   added for a real identity). The game needs no entitlements: it loads no unsigned code and does
   not JIT (PUC Lua is an interpreter).
2. `xcrun notarytool submit port/zig-out/packages/harvest-port-<version>-macos.zip --keychain-profile
   <profile> --wait` (store the credentials once with `xcrun notarytool store-credentials`).
3. `xcrun stapler staple "port/zig-out/package-work/harvest-port-<version>-macos/Harvest Port.app"`,
   then zip again with `ditto -c -k --keepParent` so the ticket ships inside.

A `.dmg` (`hdiutil create -srcfolder … -format UDZO`) would also need signing and notarizing; the zip
is enough for now.

## Windows

- `build.zig` links release builds (`-Doptimize` other than Debug) for the **GUI subsystem**, so no
  console window opens; debug builds keep the console subsystem for the log. SDL's `SDL_main`
  supplies both `main` and `WinMain`.
- [`harvest.rc`](../../port/packaging/windows/harvest.rc), compiled by zig's resource compiler, adds
  the icon (the first icon, which Explorer and SDL's window use) and a `VERSIONINFO` with the
  version from `build.zig.zon` (file and product version, description, original file name).
- The executable links statically against libc++ and SDL and imports only system DLLs and the
  Universal CRT (`api-ms-win-crt-*`), so the zip needs no DLLs next to it.
- Not signed. Signing needs an Authenticode certificate (`signtool sign /fd SHA256 /tr <timestamp
  url> /td SHA256 harvest.exe`, or `osslsigncode` from macOS or Linux); unsigned, SmartScreen warns
  on first start.

## Linux

- Built for `x86_64-linux-gnu.2.31`: SDL's process API needs glibc 2.29
  (`posix_spawn_file_actions_addchdir_np`), and 2.31 matches Ubuntu 20.04. The executable needs only
  glibc (`libc`, `libm`, `libpthread`, `libdl`); SDL and miniaudio load X11 or Wayland, OpenGL/EGL
  and the sound server's library at run time.
- [`harvest-port.desktop`](../../port/packaging/linux/harvest-port.desktop) is the menu entry;
  `install-desktop-entry.sh` copies it into `~/.local/share/applications` with `Exec` and `Path`
  pointing at the unpacked folder, and the icon into the `hicolor` theme.
- The tarball's files belong to root (uid 0) and carry no macOS metadata.
- No AppImage yet: it would need `appimagetool` (a download) and a test on real distributions;
  the tarball covers the same ground.

## The icon

[`harvest.svg`](../../port/packaging/icons/harvest.svg) is drawn for the port (crystals on a
planet's horizon); it does not use the game's artwork, which stays out of the repository and the
packages. `uv run python port/tools/make_icons.py` renders it with `rsvg-convert` into
`harvest.png` (256 px, Linux), `harvest.ico` (16–256 px, Windows) and `harvest.icns` (16–1024 px,
macOS); the outputs are committed so builds need neither.

## Checking a package

- macOS: unzip, `codesign --verify --strict --verbose=2 "Harvest Port.app"`, `lipo -info` on
  `Contents/MacOS/harvest`, then run `Contents/MacOS/harvest` under `arch -arm64` and `arch -x86_64`
  with `harvestClientData` next to the app.
- Windows: `x86_64-w64-mingw32-objdump -p harvest.exe` shows `Subsystem 2 (Windows GUI)` and the
  imported DLLs; the `.rsrc` section holds the icon and version.
- Linux: `tar tzvf`, `objdump -T harvest | grep GLIBC_` (nothing above 2.29), and a run in a
  container with the data in a fake Steam library.
