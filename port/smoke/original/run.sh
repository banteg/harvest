#!/bin/bash
# Runs inside harvest-original-linux (port/smoke/original/Dockerfile): plays the original 1.18 Linux
# build (/orig, read-only) under Xvfb with xdotool to leave the user data a player of it would have,
# a profile and a save game of a normal game, in /out/userdata, with screenshots of each step. The
# original runs on the wall clock, so the steps wait generously.
set -uo pipefail

Xvfb :99 -screen 0 1920x1080x24 -nolisten tcp > /out/xvfb.log 2>&1 &
export DISPLAY=:99
for _ in $(seq 50); do xdpyinfo > /dev/null 2>&1 && break; sleep 0.1; done
pulseaudio --daemonize=no --exit-idle-time=-1 -n --load=module-native-protocol-unix \
    --load="module-null-sink sink_name=smoke" --load=module-always-sink > /out/pulseaudio.log 2>&1 &

# the window the port opens on this desktop (1080x810), so the port's coordinates apply
mkdir -p ~/.Harvest
printf 'settings\n{\n\tlanguage = $GAME_RESOURCES$/harvestClientData/lang/english.cfg;\n\tresolutionh = 1080;\n\tresolutionv = 810;\n\tshaderlevel = 2;\n\tfullscreen = 0;\n\tdriver = OGL;\n\tvolumesfx = 3;\n\tvolumemusic = 3;\n\tparticles = 2;\n\tscrollspeed = 1.00;\n\tfirstrun = 0;\n}\n' \
    | iconv -f UTF-8 -t UTF-16 > ~/.Harvest/harvest.cfg

mkdir -p /out/screenshots
shot() { python3 -c "from PIL import ImageGrab; ImageGrab.grab(xdisplay=':99').crop($geometry).save('/out/screenshots/$1.png')"; }
click() { xdotool mousemove --window "$window" "$1" "$2" sleep 0.3 click 1; }

cd /orig
LD_LIBRARY_PATH=/orig/bin ./Harvest > /out/original.log 2>&1 &
pid=$!

# SFML sets no _NET_WM_PID; the game's is the only large child of the root window (GTK and SFML
# keep small helper windows)
for _ in $(seq 120); do
    window=$(xwininfo -root -children | awk '/^ +0x/ { split($(NF-1), size, /[x+]/); if (size[1] >= 640) { print $1; exit } }')
    [ -n "$window" ] && break
    sleep 0.5
done
xwininfo -root -children > /out/windows.txt
eval "$(xdotool getwindowgeometry --shell "$window")"
geometry="($X, $Y, $((X + WIDTH)), $((Y + HEIGHT)))"
echo "window $window at $X,$Y, ${WIDTH}x$HEIGHT" | tee /out/report.txt

sleep 12; shot 01-intro
xdotool windowfocus "$window" key space
sleep 25; shot 02-new-profile
xdotool type --delay 100 "Original Player"
sleep 1; shot 03-name-typed
click 539 454
sleep 5; shot 04-introduction-offer
click 611 537
sleep 5; shot 05-main-menu
# New Game, then the orange planet (Hephaestus): it orbits on the wall clock, so find it in a frame
click 536 703
sleep 4; shot 06-destinations
read -r px py < <(python3 /smoke/original/find_planet.py /out/screenshots/06-destinations.png)
echo "planet at $px,$py" | tee -a /out/report.txt
xdotool mousemove --window "$window" $((px - 2)) "$py" sleep 0.2 mousemove --window "$window" "$px" "$py"
sleep 0.3; xdotool click 1
sleep 6; shot 07-game-modes
xdotool mousemove --window "$window" 619 380 sleep 0.3 mousemove --window "$window" 619 390
sleep 0.5; xdotool click 1
sleep 45; shot 08-game
# Menu, Save Game, a new slot, a description
click 1025 783; sleep 4; shot 09-ingame-menu
click 540 368; sleep 4
click 441 193; sleep 2
click 721 179; sleep 4
click 538 401
xdotool type --delay 100 "Original save"
sleep 1; shot 10-save-description
click 606 433
sleep 8; shot 11-saved

# Ctrl+Q closes the device, and the game writes harvest.cfg (with the recent profile) as it exits
xdotool windowfocus "$window" key ctrl+q
for _ in $(seq 60); do kill -0 "$pid" 2> /dev/null || break; sleep 0.5; done
if kill -0 "$pid" 2> /dev/null; then
    echo "the original did not exit on Ctrl+Q" | tee -a /out/report.txt
    kill "$pid"
fi
wait "$pid" 2> /dev/null
echo "the original exited with status $?" | tee -a /out/report.txt
cp -r ~/.Harvest /out/userdata
find ~/.Harvest -type f | sort | sed "s|$HOME/||" | tee -a /out/report.txt
