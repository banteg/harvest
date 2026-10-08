# Harvest: Massive Encounter — decompilation and modern port

[![1.18](https://decomp.dev/banteg/harvest.svg?mode=shield&measure=code&label=1.18)](https://decomp.dev/banteg/harvest)

Harvest: Massive Encounter is a 2008 game by Oxeye Game Studio. This repository holds two things
built on the same recovered C++:

- a **matching decompilation** of its final patch, 1.18 (built in 2012): source that compiles back to
  the original executable, function by function;
- a **modern port** of that source to macOS, Linux, Windows and the web, built with zig on SDL3.
  It keeps the recovered game and engine code as is and replaces only the platform layer.

Neither includes the game's data: you need your own copy (the Steam release works).

## Playing the port

In the browser: open [harvest.banteg.xyz](https://harvest.banteg.xyz/). The game data downloads
automatically from the Reflexive bucket and stays in the browser for the next visit. You can also
drop your install folder (or a zip of it) onto the disc.

On the desktop, build it with [zig](https://ziglang.org) 0.17 and [just](https://just.systems):

```bash
just play                 # optimized build, then run
just run                  # debug build with UBSan, then run
just package              # release packages for macOS, Windows and Linux in port/zig-out/packages
```

The port finds the game data in the usual install locations (Steam included) or takes
`--data <folder>`; the `just` recipes pass `orig/1.18-linux-amd64`. Settings, profiles and saves go
to `~/.Harvest`, `~/Library/Application Support/Harvest` or `%APPDATA%\Harvest`. See
[port/README.md](port/README.md) for targets and options, and [docs/port/](docs/port/README.md) for
how the port works and the original bugs it found.

## The decompilation

The target is the Linux amd64 build (`1.18-linux-amd64`), compiled with GCC 4.4.3 from Ubuntu 10.04.
The Mac 1.18 build keeps its linker debug map: 264 original source files, the header names, and
names and sizes for about 6,400 functions. That map supplies the names and the source layout.
The Linux i386 and Windows 1.18 builds serve as extra references.

### Layout

```
builds.json          pinned builds: package and image sha256, compiler, role
orig/<build>/        original binaries (gitignored; checked by `hv verify`)
reference/<build>/   data generated from the reference builds (Mac debug map, vtables)
config/<build>/      matching-target configuration: symbols, splits, compiler flags
src/                 recovered C++, laid out like the original oxeye/ tree
  HarvestFull/       the game (harvest::)
  daisy/             the engine, forked from Irrlicht 0.7 (daisy::)
  ox/                interface headers and core utilities (ox::)
third_party/         Irrlicht 0.7 source (reference) and headers the game was compiled against
port/                the modern port: build.zig, SDL3 device, renderer, audio, web page
toolchain/           Ubuntu 10.04 container with the original GCC 4.4.3
tools/hv/            Python tooling (`uv run hv ...`)
tests/               tests for the tooling
```

### Setup

The original builds are not in the repository. Put them under `orig/<build>/`: for the Linux builds,
the unpacked release tarball (the executable plus `bin/` and `harvestClientData/`, so it can run);
for Mac and Windows, only the executable. Then check them against the pins:

```bash
just verify
```

The Windows and Mac images can be downloaded from pinned Steam manifests with
`uv run python tools/download-steam.py YOUR_STEAM_USERNAME --install`. The DRM-free Linux
builds remain the target and reference; the differing Steam Linux build is archived
separately. See [docs/provenance.md](docs/provenance.md) for acquisition evidence,
checksums and download instructions.

Build the toolchain image (needs Docker or Podman, including rootless Podman on SELinux hosts; on
Apple Silicon it runs under x86-64 emulation):

```bash
just toolchain
```

`just shell` opens a shell in it with the repository at `/work`; `just tc g++ --version` runs one
command without a terminal. `toolchain/manifest.tsv` lists the image's installed packages
(`just toolchain-manifest`); rebuilds from the frozen lucid release pocket should reproduce it.

### Reference data

`reference/1.18-mac-i386/` is generated from the Mac executable by `just import-mac` and is committed:

- `units.csv`: the 264 source files in link order, with object names and function totals
- `functions.csv`: 6,406 functions with address, size, source file, ELF-style mangled name and demangled name
- `symbols.csv`: every defined symbol, including data, vtables and typeinfo. A symbol gets a source
  file from its own debug record (matched by address and name, since local names like `_GLOBAL__I_a`
  repeat across files), or by name for addressless globals that only one file claims.
- `vtables.csv`: the raw pointer-sized words from each of the 354 vtable symbols up to the next symbol,
  resolved to symbols. Address points, secondary tables and padding are not classified yet.
- `headers.csv`: the header markers (`N_SOL`) seen inside each source file's debug records: headers
  that contributed emitted code, not the full include graph

### Matching

`just port-symbols` names the target's RTTI, vtables and virtual functions from the Mac vtables into
`config/1.18-linux-amd64/symbols.tsv`. `just match` compiles every source listed in
`config/1.18-linux-amd64/units.toml` and compares each object with the Linux executable, section by
section, with every relocation resolved. It also writes delinked target objects and `objdiff.json` for
objdiff: `just objdiff-cli` installs the pinned CLI, and `just diff <unit> <symbol>` shows one
function's differences. See [docs/matching.md](docs/matching.md).

`hv search <unit>` performs bounded definition-order experiments over the unit's functions, with cached
canonical compilations, saved patches and a guard against losing exact matches. See
[docs/search.md](docs/search.md) for block selection, batching and optional application.
