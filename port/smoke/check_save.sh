#!/bin/sh
# Check the save the port actually resumed, optionally against the original game's input save.
set -eu
profiles=$1
log=$2
for save in "$profiles"/*.hsg; do
    [ -f "$save" ] && [ -s "$save" ] || continue
    name=${save##*/}
    if [ "$#" -eq 3 ] && ! cmp -s "$save" "$3/$name"; then
        continue
    fi
    if grep -Fxq "Loaded save game: \$HARVEST_USERDATA\$/profiles/$name" "$log"; then
        echo "loaded save game: $name"
        exit 0
    fi
done
echo "no successful load of a nonempty save from ${3:-$profiles}"
exit 1
