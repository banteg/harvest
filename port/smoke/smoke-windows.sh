#!/usr/bin/env bash
# The Windows smoke test: cross-builds the port for x86_64-windows-gnu, runs the text test
# (port/tests/text.cpp) and then the game under Wine with port/smoke/windows.input, which goes from
# the intro through a new profile, the main menu, the settings and the profile list into the
# tutorial game and takes a screenshot at each step. Fails on a build error, a crash or UBSan stop,
# a non-zero exit, error output, a missing screenshot or a blank one.
#
#   port/smoke/smoke-windows.sh [data dir]     (just smoke-windows)
#
# The data dir holds harvestClientData/ (default: orig/1.18-linux-amd64). Screenshots and logs go to
# build/smoke/windows/. Wine: $WINE, else Wine Stable's app bundle, else wine on the PATH; it runs in
# its own prefix, $WINEPREFIX or build/wine-prefix, created on first use. Each run starts with no user
# data (%APPDATA%\Harvest).
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
data="$(cd "${1:-$root/orig/1.18-linux-amd64}" && pwd)"
out="$root/build/smoke/windows"
input="$root/port/smoke/windows.input"

if [ -n "${WINE:-}" ]; then
    wine="$WINE"
elif [ -x "/Applications/Wine Stable.app/Contents/Resources/wine/bin/wine" ]; then
    wine="/Applications/Wine Stable.app/Contents/Resources/wine/bin/wine"
else
    wine="$(command -v wine)" || { echo "no wine found (set WINE)" >&2; exit 1; }
fi
timeout="$(command -v timeout || command -v gtimeout)" || { echo "needs timeout (coreutils)" >&2; exit 1; }

export WINEPREFIX="${WINEPREFIX:-$root/build/wine-prefix}"
# no Wine debug channels; no Mono or Gecko install prompts when the prefix is created
export WINEDEBUG=-all WINEDLLOVERRIDES="mscoree,mshtml="

echo "== build"
(cd "$root/port" && zig build harvest tests -Dtarget=x86_64-windows-gnu)
bin="$root/port/zig-out/bin"

if [ ! -f "$WINEPREFIX/system.reg" ]; then
    echo "== new Wine prefix in $WINEPREFIX"
    "$wine" wineboot -i > /dev/null 2>&1
fi

rm -rf "$out"
mkdir -p "$out"
userdata="$WINEPREFIX/drive_c/users/$(id -un)/AppData/Roaming/Harvest"
rm -rf "$userdata"

# Wine maps / to Z:
windows_path() { echo "Z:$1"; }

# runs a Windows program with a time limit, its output in a log; stops on a failure
run() {
    local log="$1" limit="$2"
    shift 2
    local status=0
    "$timeout" "$limit" "$wine" "$@" > "$out/$log.raw" 2>&1 || status=$?
    # MoltenVK's information lines (Wine's Vulkan on macOS) are not the program's
    grep -av -e '^\[mvk-info\]' -e '^	' "$out/$log.raw" > "$out/$log" || true
    rm "$out/$log.raw"
    if [ "$status" -ne 0 ]; then
        tail -n 40 "$out/$log" >&2
        echo "FAIL: $* exited with $status (log: $out/$log)" >&2
        exit 1
    fi
}

echo "== text test"
mkdir -p "$out/text-scratch"
run text.log 120 "$bin/harvest-test-text.exe" "$(windows_path "$data")" "$(windows_path "$out/text-scratch")"
tail -n 1 "$out/text.log"

echo "== game"
run harvest.log 600 "$bin/harvest.exe" --data "$(windows_path "$data")" --input-script "$(windows_path "$input")"

# error output: UBSan stops, failed loads and other complaints (the GLES attempt before OpenGL 3.3 is
# expected where the driver has no OpenGL ES)
errors="$(grep -ai -e 'panic' -e 'runtime error' -e 'error' -e 'fail' -e 'could not' -e 'cannot' -e 'unable' \
    -e 'not found' "$out/harvest.log" | grep -av 'cannot create a OpenGL ES 3.0 window' || true)"
if [ -n "$errors" ]; then
    echo "$errors" >&2
    echo "FAIL: error output (log: $out/harvest.log)" >&2
    exit 1
fi

# the play state loaded (its effects and particles are not loaded before)
if ! grep -aq 'harvestClientData/gfx/fx.dat' "$out/harvest.log"; then
    echo "FAIL: the game never started (log: $out/harvest.log)" >&2
    exit 1
fi

# the screenshots, renamed without their date: <name>-<yymmdd>-NN.jpg -> <name>.jpg
names=($(awk '$2 == "screenshot" { print $3 }' "$input"))
shots=()
for name in "${names[@]}"; do
    file="$(ls "$userdata/screenshots/$name"-*.jpg 2>/dev/null | head -n 1 || true)"
    if [ -n "$file" ]; then
        cp "$file" "$out/$name.jpg"
    fi
    shots+=("$out/$name.jpg")
done

echo "== screenshots"
uv run --quiet --no-project --with pillow python "$root/port/smoke/check_screenshots.py" "${shots[@]}"
echo "ok: $out"
