#!/usr/bin/env bash
# The port's Linux smoke test: cross-builds the game for Linux with zig, runs it in a Debian
# container (port/smoke/Dockerfile: Xvfb or headless Weston, Mesa's llvmpipe, PulseAudio, PipeWire
# or ALSA without a sound card) driven by an input script, and checks that it does not crash or
# trip UBSan, that it gets the OpenGL context and audio backend asked for, and that every screenshot
# the script takes exists and is not blank. Results go to build/smoke/linux/<configuration>/.
#
#   port/smoke/smoke-linux.sh [--arch aarch64|x86_64] [--display x11|wayland]
#       [--audio pulse|pipewire|alsa|none] [--gl es|core] [--script <file in port/smoke>]
#       [--optimize Debug|ReleaseSafe|ReleaseFast] [--no-build] [--matrix] [--original]
#
# --arch defaults to the Docker host's architecture (the other one runs under emulation).
# --matrix runs the configurations in MATRIX below instead of one. --original then also plays the
# original 1.18 Linux build (x86-64, in port/smoke/original/Dockerfile) to make a profile and a save
# game, and runs the port on that ~/.Harvest with original.script, which loads the save. The game
# data is mounted read-only from $HARVEST_DATA or orig/1.18-linux-amd64 (the original's executable
# too); nothing of it goes into an image.
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
data="$(cd "${HARVEST_DATA:-$root/orig/1.18-linux-amd64}" && pwd -P)"

case "$(docker info --format '{{.Architecture}}')" in
aarch64 | arm64) arch=aarch64 ;;
*) arch=x86_64 ;;
esac
display=x11 audio=pulse gl=es script=menu.script optimize=Debug build=1 matrix=0 original=0

while [ $# -gt 0 ]; do
    case "$1" in
    --arch) arch=$2; shift ;;
    --display) display=$2; shift ;;
    --audio) audio=$2; shift ;;
    --gl) gl=$2; shift ;;
    --script) script=$2; shift ;;
    --optimize) optimize=$2; shift ;;
    --no-build) build=0 ;;
    --matrix) matrix=1 ;;
    --original) original=1 ;;
    *) sed -n '2,/^set -euo/p' "$0" | sed '$d; s/^# \{0,1\}//' >&2; exit 2 ;;
    esac
    shift
done

# arch display audio gl: X11 and Wayland, each sound path, both OpenGL profiles, and the other
# architecture once under emulation
MATRIX="
aarch64 x11 pulse es
aarch64 x11 alsa core
aarch64 wayland pipewire es
x86_64 x11 pulse es
"

[ -d "$data/harvestClientData" ] || { echo "no harvestClientData in $data (set HARVEST_DATA)" >&2; exit 1; }

# Empties a results directory in place: Docker Desktop can keep a bind mount on a directory deleted
# and made again, and the container then writes nowhere.
clean() {
    mkdir -p "$1" && find "$1" -mindepth 1 -delete
}

built=""
prepare() {
    local arch=$1
    case " $built " in *" $arch "*) return ;; esac
    built="$built $arch"
    if [ "$build" = 1 ]; then
        echo "== building the port for $arch-linux-gnu ($optimize)"
        (cd "$root/port" && zig build harvest tests -Dtarget="$arch-linux-gnu" -Doptimize="$optimize" \
            --prefix "zig-out/$arch-linux")
    fi
    local platform=linux/arm64
    [ "$arch" = x86_64 ] && platform=linux/amd64
    echo "== building the container image harvest-smoke-linux:$arch"
    docker build --quiet --platform "$platform" -t "harvest-smoke-linux:$arch" "$root/port/smoke" > /dev/null
}

# run <arch> <display> <audio> <gl> [<script> <user data directory>]
run() {
    local arch=$1 display=$2 audio=$3 gl=$4 script=${5:-$script} userdata=${6:-}
    local name="$arch-$display-$audio-$gl"
    [ -n "$userdata" ] && name="$name-original"
    local out="$root/build/smoke/linux/$name"
    prepare "$arch"
    clean "$out"
    echo "== $name"
    local platform=linux/arm64
    [ "$arch" = x86_64 ] && platform=linux/amd64
    local mounts=(-v "$root/port/zig-out/$arch-linux/bin:/game:ro" -v "$data:/data:ro"
        -v "$root/port/smoke:/smoke:ro" -v "$out:/out")
    [ -n "$userdata" ] && mounts+=(-v "$userdata:/userdata:ro")
    docker run --rm --platform "$platform" --shm-size=512m \
        -e DISPLAY_SERVER="$display" -e AUDIO="$audio" -e GL="$gl" -e SCRIPT="$script" \
        "${mounts[@]}" "harvest-smoke-linux:$arch" /smoke/run.sh
}

# plays the original build to build/smoke/linux/original (screenshots, userdata/)
play_original() {
    local out="$root/build/smoke/linux/original"
    echo "== building the container image harvest-original-linux"
    docker build --quiet --platform linux/amd64 -t harvest-original-linux "$root/port/smoke/original" > /dev/null
    clean "$out"
    echo "== the original 1.18 Linux build"
    docker run --rm --platform linux/amd64 -v "$data:/orig:ro" -v "$root/port/smoke:/smoke:ro" -v "$out:/out" \
        harvest-original-linux /smoke/original/run.sh
    ls "$out"/userdata/profiles/*.hsg > /dev/null 2>&1 || { echo "the original left no save game" >&2; return 1; }
}

status=0
if [ "$matrix" = 1 ]; then
    summary=""
    while read -r a d s g; do
        [ -n "$a" ] || continue
        if run "$a" "$d" "$s" "$g"; then result=PASS; else result=FAIL; status=1; fi
        summary="$summary$result $a-$d-$s-$g"$'\n'
    done <<< "$MATRIX"
    printf '\n%s' "$summary"
else
    run "$arch" "$display" "$audio" "$gl" || status=1
fi
if [ "$original" = 1 ]; then
    if play_original && run "$arch" "$display" "$audio" "$gl" original.script "$root/build/smoke/linux/original/userdata"
    then echo "PASS the port on the original's user data"
    else echo "FAIL the port on the original's user data"; status=1
    fi
fi
echo "results in $root/build/smoke/linux/"
exit "$status"
