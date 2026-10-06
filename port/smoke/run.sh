#!/bin/bash
# Runs inside the smoke-test container (port/smoke/Dockerfile); port/smoke/smoke-linux.sh starts it.
# Mounts: /game (the Linux build's bin/), /data (the game data, read-only), /smoke (this folder),
# /out (results). Settings come from the environment:
#   DISPLAY_SERVER  x11 (Xvfb) or wayland (headless Weston)
#   AUDIO           pulse (PulseAudio with a null sink), pipewire (PipeWire, its PulseAudio server
#                   and a null sink), alsa (ALSA's null device, no sound server) or none (--no-audio)
#   GL              es (the device's first choice) or core (Mesa refuses ES 3.0, so the device
#                   falls back to OpenGL 3.3 core)
#   SCRIPT          the input script in /smoke (default menu.script)
#   TIMEOUT         seconds before the game is killed (default 600)
#   /userdata       if mounted, copied to ~/.Harvest first (the original build's user data)
# Exits non-zero when the game or a check fails; /out/report.txt lists what was checked.
set -uo pipefail

DISPLAY_SERVER=${DISPLAY_SERVER:-x11}
AUDIO=${AUDIO:-pulse}
GL=${GL:-es}
SCRIPT=${SCRIPT:-menu.script}
TIMEOUT=${TIMEOUT:-600}

report=/out/report.txt
: > "$report"
failed=0
note() { echo "$*" | tee -a "$report"; }
fail() { note "FAIL: $*"; failed=1; }

# --- the desktop ----------------------------------------------------------------------------------

case "$DISPLAY_SERVER" in
x11)
    Xvfb :99 -screen 0 1920x1080x24 -nolisten tcp > /out/xvfb.log 2>&1 &
    export DISPLAY=:99 SDL_VIDEO_DRIVER=x11
    for _ in $(seq 50); do xdpyinfo > /dev/null 2>&1 && break; sleep 0.1; done
    ;;
wayland)
    weston --backend=headless --renderer=gl --width=1920 --height=1080 --socket=wayland-1 --idle-time=0 \
        > /out/weston.log 2>&1 &
    export WAYLAND_DISPLAY=wayland-1 SDL_VIDEO_DRIVER=wayland
    for _ in $(seq 50); do [ -S "$XDG_RUNTIME_DIR/wayland-1" ] && break; sleep 0.1; done
    ;;
*) echo "unknown DISPLAY_SERVER $DISPLAY_SERVER" >&2; exit 2 ;;
esac

# the clipboard a script can paste with Ctrl+V (xclip stays to serve it). Headless Weston has no
# seat, so Wayland has no clipboard here.
clipboard=""
if [ "$DISPLAY_SERVER" = x11 ]; then
    clipboard=" Test"
    printf %s "$clipboard" | xclip -selection clipboard
fi

# --- the sound server -----------------------------------------------------------------------------

game_args=()
case "$AUDIO" in
pulse)
    pulseaudio --daemonize=no --exit-idle-time=-1 --disallow-exit -n \
        --load=module-native-protocol-unix --load="module-null-sink sink_name=smoke" \
        --load=module-always-sink > /out/pulseaudio.log 2>&1 &
    for _ in $(seq 50); do pactl info > /dev/null 2>&1 && break; sleep 0.1; done
    pactl set-default-sink smoke
    ;;
pipewire)
    # PipeWire needs a session bus for WirePlumber
    dbus-daemon --session --address="unix:path=$XDG_RUNTIME_DIR/bus" --fork --nopidfile
    export DBUS_SESSION_BUS_ADDRESS="unix:path=$XDG_RUNTIME_DIR/bus"
    pipewire > /out/pipewire.log 2>&1 &
    wireplumber > /out/wireplumber.log 2>&1 &
    pipewire-pulse > /out/pipewire-pulse.log 2>&1 &
    for _ in $(seq 50); do pactl info > /dev/null 2>&1 && break; sleep 0.1; done
    pactl load-module module-null-sink sink_name=smoke > /dev/null
    pactl set-default-sink smoke
    ;;
alsa)
    # no card and no server: ALSA's default device discards the sound
    printf 'pcm.!default { type null }\nctl.!default { type hw card 0 }\n' > ~/.asoundrc
    ;;
none)
    game_args+=(--no-audio)
    ;;
*) echo "unknown AUDIO $AUDIO" >&2; exit 2 ;;
esac

# --- OpenGL ---------------------------------------------------------------------------------------

case "$GL" in
es) ;;
core) export MESA_GLES_VERSION_OVERRIDE=2.0 ;;
*) echo "unknown GL $GL" >&2; exit 2 ;;
esac

note "smoke test: $(uname -m), $DISPLAY_SERVER, audio $AUDIO, GL $GL, script $SCRIPT"

# --- user data the original Linux build would leave -----------------------------------------------

mkdir -p ~/.Harvest
if [ -d /userdata ]; then
    cp -r /userdata/. ~/.Harvest/
    note "user data from /userdata: $(cd /userdata && find . -type f | sed 's|^\./||' | sort | tr '\n' ' ')"
fi

# Loose user mods whose names sort differently in byte order (the port) and in en_US (the original):
# B, Z, a, z against a, B, z, Z.
mkdir -p ~/.Harvest/mods
for mod in alpha Bravo zulu Zeta; do
    mkdir -p ~/.Harvest/mods/$mod
    printf -- '-- smoke test mod\n' > ~/.Harvest/mods/$mod/main.lua
    printf 'mod\n{\n\tname = %s (user mod);\n\tfolder = %s;\n\tauthor = smoke test;\n}\n' $mod $mod \
        > ~/.Harvest/mods/$mod.hmd
done

# --- the run --------------------------------------------------------------------------------------

start=$(date +%s)
timeout --signal=KILL "$TIMEOUT" /game/harvest --data /data --input-script "/smoke/$SCRIPT" "${game_args[@]}" \
    > /out/game.log 2>&1
status=$?
note "game exited with status $status after $(($(date +%s) - start)) s"

cp -r ~/.Harvest /out/userdata
find ~/.Harvest -type f | sort | sed "s|$HOME/||" > /out/userdata.txt

# --- checks ---------------------------------------------------------------------------------------

[ "$status" -eq 0 ] || fail "the game exited with status $status (124/137: timeout, 132+: a signal)"

# zig's UBSan runtime reports "panic: <check>" with a stack; crashes print "Segmentation fault" etc.
if grep -nE 'panic:|runtime error|Segmentation fault|Illegal instruction|Aborted|AddressSanitizer' /out/game.log \
    > /out/crash.txt; then
    fail "the log reports a crash or a UBSan check:"
    head -5 /out/crash.txt | tee -a "$report"
fi

context=$(grep -m1 -E '^OpenGL (ES|core) context:' /out/game.log)
note "${context:-no OpenGL context line}"
case "$GL" in
es) [[ "$context" == "OpenGL ES context:"* ]] || fail "expected an OpenGL ES context" ;;
core) [[ "$context" == "OpenGL core context:"* ]] || fail "expected the OpenGL 3.3 core fallback" ;;
esac

audio=$(grep -m1 -E '^audio: |cannot (open|start)' /out/game.log)
note "${audio:-no audio line}"
case "$AUDIO" in
pulse | pipewire) [[ "$audio" == "audio: PulseAudio"* ]] || fail "expected miniaudio's PulseAudio backend" ;;
alsa) [[ "$audio" == "audio: ALSA"* ]] || fail "expected miniaudio's ALSA backend" ;;
none) [ -z "$audio" ] || fail "--no-audio opened an audio device" ;;
esac

[ -f ~/.Harvest/harvest.cfg ] || fail "~/.Harvest/harvest.cfg was not written"
profile=$(cat ~/.Harvest/profiles/*.cfg 2>/dev/null | iconv -f UTF-16 -t UTF-8 | grep -m1 -E '^\s*name = ')
note "profile: ${profile:-none}"
[ -n "$profile" ] || fail "no profile with a name in ~/.Harvest/profiles"
# a script that pastes into the name expects the clipboard at its end
if [ -n "$clipboard" ] && grep -qE '^[0-9]+ key ctrl\+v' "/smoke/$SCRIPT"; then
    [[ "$profile" == *"$clipboard;" ]] || fail "Ctrl+V did not paste \"$clipboard\" into the profile name"
fi

# every "screenshot <name>" of the script must exist and show something
mkdir -p /out/screenshots
for name in $(awk '$2 == "screenshot" { print $3 }' "/smoke/$SCRIPT"); do
    shot=$(ls ~/.Harvest/screenshots/"$name"-*.jpg 2>/dev/null | head -1)
    if [ -z "$shot" ]; then
        fail "screenshot $name is missing"
        continue
    fi
    cp "$shot" "/out/screenshots/$name.jpg"
    verdict=$(python3 /smoke/check_image.py "/out/screenshots/$name.jpg")
    note "screenshot $name: $verdict"
    [[ "$verdict" == ok* ]] || fail "screenshot $name looks blank"
done

# Ctrl+T, the game's own screenshot key, writes Screen-<yymmdd>-NN.jpg
if grep -qE '^[0-9]+ key ctrl\+t' "/smoke/$SCRIPT"; then
    if ls ~/.Harvest/screenshots/Screen-*.jpg > /dev/null 2>&1; then
        note "Ctrl+T: $(cd ~/.Harvest/screenshots && ls Screen-*.jpg | head -1)"
    else
        fail "Ctrl+T saved no Screen-*.jpg"
    fi
fi
saves=$(cd ~/.Harvest/profiles && ls *.hsg 2> /dev/null | tr '\n' ' ')
note "save games: ${saves:-none}"

# the renderer test (port/tests/video.cpp) in both profiles, without the GL override: its 2D, 3D and
# menu scenes compare directly with `just port-test video --screenshot <dir>` on macOS
unset MESA_GLES_VERSION_OVERRIDE
if [ -x /game/harvest-test-video ]; then
    for profile in core gles; do
        flags=()
        [ "$profile" = gles ] && flags=(--gles)
        mkdir -p "/out/video-$profile"
        if ! timeout --signal=KILL 300 /game/harvest-test-video /data --screenshot "/out/video-$profile" \
            "${flags[@]}" > "/out/video-$profile.log" 2>&1; then
            fail "the video test ($profile) failed: $(tail -1 "/out/video-$profile.log")"
            continue
        fi
        for scene in 2d 3d menu; do
            shot=$(ls "/out/video-$profile"/video-test-$scene-*.jpg 2>/dev/null | head -1)
            if [ -z "$shot" ]; then
                fail "video test ($profile): no $scene screenshot"
                continue
            fi
            mv "$shot" "/out/video-$profile/$scene.jpg"
            verdict=$(python3 /smoke/check_image.py "/out/video-$profile/$scene.jpg")
            note "video test $profile $scene: $verdict"
            [[ "$verdict" == ok* ]] || fail "video test ($profile) $scene looks blank"
        done
    done
fi

if [ "$failed" -eq 0 ]; then note "PASS"; else note "the smoke test FAILED"; fi
exit "$failed"
