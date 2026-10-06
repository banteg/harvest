image := "harvest-toolchain"

# list recipes
default:
    @just --list

# check orig/ images against builds.json pins
verify:
    uv run hv verify

# regenerate reference tables from the Mac debug map
import-mac:
    uv run hv import-mac

# download the pinned objdiff-cli into build/tools
objdiff-cli:
    #!/usr/bin/env bash
    set -euo pipefail
    platform="$(uname -sm)"
    # just may run under Rosetta on Apple Silicon, where uname reports x86_64
    if [ "$(uname -s)" = Darwin ] && [ "$(sysctl -in hw.optional.arm64)" = 1 ]; then platform="Darwin arm64"; fi
    case "$platform" in
      "Darwin arm64") asset=objdiff-cli-macos-arm64; sha=98f8275c27900c4fe2248fce3af37617658be49648fa7dbb5b376371f046dfdb ;;
      "Linux x86_64") asset=objdiff-cli-linux-x86_64; sha=c8290281e82114bcc1a06ff73061110d3902a177822e750337de2537188e358f ;;
      *) echo "no pinned objdiff-cli for $(uname -sm)" >&2; exit 1 ;;
    esac
    mkdir -p build/tools
    curl -fsSL -o build/tools/objdiff-cli.tmp "https://github.com/encounter/objdiff/releases/download/v3.8.1/$asset"
    echo "$sha  build/tools/objdiff-cli.tmp" | shasum -a 256 -c -
    chmod +x build/tools/objdiff-cli.tmp && mv build/tools/objdiff-cli.tmp build/tools/objdiff-cli

# download the pinned zig for the port into build/tools/zig
zig:
    #!/usr/bin/env bash
    set -euo pipefail
    version=0.17.0
    platform="$(uname -sm)"
    # just may run under Rosetta on Apple Silicon, where uname reports x86_64
    if [ "$(uname -s)" = Darwin ] && [ "$(sysctl -in hw.optional.arm64)" = 1 ]; then platform="Darwin arm64"; fi
    case "$platform" in
      "Darwin arm64") target=aarch64-macos; sha=b607e9b9234790a008116ae5bdb71c6243b84b9fb42a53a9e70fde41c06c536a ;;
      "Darwin x86_64") target=x86_64-macos; sha=4f9a1c5269aa17ebda5e6d3c2b89d6cbf36f7d2b22a0306e9ab98f25f95529c6 ;;
      "Linux x86_64") target=x86_64-linux; sha=1cbe9df9f27e6b78d14ccbca43b6703a404ef79ef1c463de901d7f088d4e2026 ;;
      "Linux aarch64") target=aarch64-linux; sha=9e8d11661d4ae3bd57702a3832781e23ad151dde5798e16a5ccd503f65234ff8 ;;
      *) echo "no pinned zig for $(uname -sm)" >&2; exit 1 ;;
    esac
    mkdir -p build/tools
    curl -fsSL -o build/tools/zig.tar.xz "https://ziglang.org/download/$version/zig-$target-$version.tar.xz"
    echo "$sha  build/tools/zig.tar.xz" | shasum -a 256 -c -
    rm -rf build/tools/zig && mkdir build/tools/zig
    tar -xJf build/tools/zig.tar.xz -C build/tools/zig --strip-components 1 && rm build/tools/zig.tar.xz
    build/tools/zig/zig version

# objdiff one function of a unit, target on the left (run match first)
diff unit symbol:
    uv run hv diff {{unit}} {{symbol}}

# name target RTTI, vtables and virtual functions from the Mac vtables
port-symbols:
    uv run hv port-symbols

# compile recovered units and compare them with the target (default: all)
match *units:
    uv run hv match {{units}}

# export an objdiff-v2 report from fresh committed evidence (no originals or Docker)
progress:
    uv run python -m hv.progress report

# recapture evidence after matching/source/tool changes (needs originals and Docker)
progress-capture:
    uv run python -m hv.progress capture

# build the lucid GCC 4.4.3 container
toolchain:
    #!/usr/bin/env bash
    set -euo pipefail
    flags=()
    # only Podman reports these fields, and only Podman's build takes the flags below
    if security="$(docker info --format '{{{{.Host.Security.Rootless}} {{{{.Host.Security.SELinuxEnabled}}' 2>/dev/null)"; then
      read -r rootless selinux <<< "$security"
      # debootstrap falls back to bind-mounting /dev nodes when it cannot mknod them
      if [ "$rootless" = true ]; then flags+=(--cap-add=SYS_ADMIN); fi
      # lucid's libselinux still thinks SELinux is on, and container_t denies its setfscreatecon
      if [ "$selinux" = true ]; then flags+=(--security-opt=label=disable); fi
    fi
    docker build ${flags[@]+"${flags[@]}"} --platform linux/amd64 -t {{image}} toolchain

# interactive shell in the toolchain container, repo at /work
shell:
    docker run --rm -it --platform linux/amd64 --security-opt=label=disable -v "{{justfile_directory()}}:/work" {{image}} bash

# run a command in the toolchain container without a tty, repo at /work
tc +cmd:
    docker run --rm --platform linux/amd64 --security-opt=label=disable -v "{{justfile_directory()}}:/work" {{image}} {{cmd}}

# record the toolchain image's installed packages
toolchain-manifest:
    docker run --rm --platform linux/amd64 {{image}} dpkg-query -W -f '${Package}\t${Version}\n' > toolchain/manifest.tsv

test:
    uv run pytest

# check lint and formatting without changing files
lint:
    uv run ruff check tools tests
    uv run ruff format --check tools tests

# apply lint fixes and formatting
fix:
    uv run ruff check tools tests --fix --unsafe-fixes
    uv run ruff format tools tests

# the port: game data defaults to the original Linux install in orig/
data := justfile_directory() / "orig/1.18-linux-amd64"

# build the port (debug, with UBSan; pass e.g. -Doptimize=ReleaseFast or -Dtarget=x86_64-windows-gnu)
build *args:
    cd port && zig build harvest {{args}}

# build and run the debug port against the original data (extra args go to the game, e.g. --no-audio)
run *args: build
    port/zig-out/bin/harvest --data "{{data}}" {{args}}

# build and run an optimized port, without UBSan stops, for playing
play *args:
    cd port && zig build harvest -Doptimize=ReleaseFast
    port/zig-out/bin/harvest --data "{{data}}" {{args}}

# build the port for every target from this machine, plus an optimized host build
# (optimized builds inline more, so they catch inline functions that are declared but never defined)
build-all: web
    cd port && zig build harvest
    cd port && zig build harvest -Doptimize=ReleaseFast
    cd port && zig build harvest -Dtarget=x86_64-linux-gnu -p zig-out/x86_64-linux
    cd port && zig build harvest -Dtarget=x86_64-windows-gnu -p zig-out/x86_64-windows

# build the web port into port/zig-out/web (needs Emscripten: em-config and em++ on PATH)
web *args:
    cd port && zig build harvest -Dtarget=wasm32-emscripten -Doptimize=ReleaseFast -p zig-out/web {{args}}

# serve the web port on localhost; drop your game folder onto the page
serve-web port="8000": web
    uv run python -m http.server {{port}} --bind 127.0.0.1 -d port/zig-out/web

# build release packages of the port into port/zig-out/packages: macos, windows, linux or all
package target="all":
    bash port/tools/package.sh {{target}}

# run one of the port's tests (audio, video, menu_scene) with the original data
port-test name *args:
    #!/usr/bin/env bash
    set -euo pipefail
    # the audio test takes the sound folder, the others the data root
    path="{{data}}"
    if [ "{{name}}" = audio ]; then path="$path/harvestClientData/sfx/"; fi
    cd port && zig build "test-{{name}}" -- "$path" {{args}}

# build the Windows port and run it under Wine through the menus into a game (screenshots in build/smoke/windows)
smoke-windows *args:
    port/smoke/smoke-windows.sh {{args}}

# cross-build the port for Linux and smoke-test it in a container (port/smoke/smoke-linux.sh --help)
smoke-linux *args:
    port/smoke/smoke-linux.sh {{args}}

# show the port's unresolved-symbol census
census:
    uv run python port/tools/census.py
