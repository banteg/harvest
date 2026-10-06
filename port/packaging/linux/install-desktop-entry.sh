#!/bin/sh
# Adds the game to the desktop's application menu for this user: installs harvest-port.desktop,
# pointing at the harvest executable in this folder, and its icon. Run it again after moving the
# folder; delete ~/.local/share/applications/harvest-port.desktop to undo.
set -eu
here="$(cd "$(dirname "$0")" && pwd)"
data="${XDG_DATA_HOME:-$HOME/.local/share}"
mkdir -p "$data/applications" "$data/icons/hicolor/256x256/apps"
cp "$here/harvest-port.png" "$data/icons/hicolor/256x256/apps/harvest-port.png"
# Exec's quoted argument: \ " ` $ take two backslashes in the file, % is doubled.
exec_path="$(printf '%s' "$here/harvest" | sed -e 's/[\\"`$]/\\\\&/g' -e 's/%/%%/g')"
{
    grep -v '^Exec=' "$here/harvest-port.desktop"
    printf 'Exec="%s"\n' "$exec_path"
    printf 'Path=%s\n' "$here"
} > "$data/applications/harvest-port.desktop"
echo "installed $data/applications/harvest-port.desktop"
