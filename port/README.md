# Harvest port

The modern port compiles the recovered C++ in [`../src`](../src) with `zig c++` and supplies its own
platform layer. The plan is in [`docs/port/porting-plan.md`](../docs/port/porting-plan.md); what the
recovered code still needs from the port is in [`docs/port/census.md`](../docs/port/census.md).

## Building

Needs zig 0.17.0 (`just zig` downloads the pinned one into `build/tools/zig`). From this directory:

    zig build                      # libharvest.a in zig-out/lib, for the host
    zig build -Doptimize=ReleaseFast
    zig build census               # links every kept unit; fails, listing the unresolved symbols

`zig build census` is expected to fail until the platform layer exists: its linker errors are the
census. For the grouped report with callers, from the repository root:

    uv run python port/tools/census.py            # the port's unresolved symbols, by owner
    uv run python port/tools/census.py --libc     # plus the C library calls the kept units make
    uv run python port/tools/census.py --original # the matching build's objects (run `just match` first)

## What is compiled

Every unit in [`config/1.18-linux-amd64/units.toml`](../config/1.18-linux-amd64/units.toml) except the
platform units listed in `replaced` in [`build.zig`](build.zig): the Linux device, OS operator, logger
and os helpers (`daisy/other/`), the joystick drivers (`daisy/input/`), the OpenAL backend, the OpenGL
driver and its material renderers, the Cg material renderer base, the software z-buffer, and
`HarvestFull/main.cpp` (the port runs the frame step from SDL3's main callbacks). Newly recovered
units join the build automatically.

Flags: `-std=gnu++98` (GCC 4.4's default dialect, which the recovered code needs no changes for) and
`-DHARVEST_PORT`, which guards the few port-only lines in shared sources. Debug builds keep zig's
UBSan (trap mode); nothing in the kept units trips it at compile time.

## Source rules

`src/` is shared with the matching build and must stay byte-identical under GCC 4.4 (`just match`).
Port-only code in a shared file goes inside `#ifdef HARVEST_PORT`; prefer replacing a whole platform
unit here over patching one.
