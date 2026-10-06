#!/usr/bin/env bash
# Builds release packages of the port into port/zig-out/packages (see docs/port/packaging.md).
#
#   package.sh macos     universal (arm64 + x86_64) app bundle, ad-hoc signed, zipped (macOS only)
#   package.sh windows   x86_64 GUI executable, zipped
#   package.sh linux     x86_64 executable, .desktop entry and icon, as a .tar.gz
#   package.sh all       every package this machine can build
#
# Packages hold the port's program, its README and the linked libraries' licences, never game data.
# Environment: HARVEST_CODESIGN_IDENTITY signs the app with a real identity instead of ad hoc
# ("-"); HARVEST_BUNDLE_ID overrides the bundle identifier.
set -euo pipefail

port="$(cd "$(dirname "$0")/.." && pwd)"
packaging="$port/packaging"
out="$port/zig-out/packages"
work="$port/zig-out/package-work"
version="$(sed -n 's/^ *\.version = "\(.*\)",$/\1/p' "$port/build.zig.zon")"
name="harvest-port-$version"

# The oldest systems the packages run on (SDL needs glibc 2.29 for posix_spawn's chdir).
minimum_macos=11.0
minimum_glibc=2.31

# build <zig target> <prefix>: an optimized, stripped game executable and the licences, under <prefix>
build() {
    (cd "$port" && zig build harvest licenses -Doptimize=ReleaseFast -Dstrip=true -Dtarget="$1" --prefix "$2")
}

# stage <dir> <prefix>: a fresh package folder with the README and the licences
stage() {
    rm -rf "$1"
    mkdir -p "$1"
    cp "$packaging/README.txt" "$1/README.txt"
    cp -R "$2/share/licenses" "$1/licenses"
}

zip_folder() { # <folder> <zip>: the folder itself at the top of the archive
    rm -f "$2"
    (cd "$(dirname "$1")" && python3 -m zipfile -c "$2" "$(basename "$1")")
}

package_macos() {
    [ "$(uname -s)" = Darwin ] || { echo "the macOS package needs macOS (lipo, codesign, the SDK)" >&2; exit 1; }
    local arm="$work/macos-aarch64" intel="$work/macos-x86_64"
    build "aarch64-macos.$minimum_macos" "$arm"
    build "x86_64-macos.$minimum_macos" "$intel"

    local folder="$work/$name-macos"
    stage "$folder" "$arm"
    # Not Harvest.app: that is the original game's name, and the port must not replace it (and the
    # data in it) when both go into /Applications.
    local app="$folder/Harvest Port.app"
    mkdir -p "$app/Contents/MacOS" "$app/Contents/Resources"
    lipo -create -output "$app/Contents/MacOS/harvest" "$arm/bin/harvest" "$intel/bin/harvest"
    sed -e "s/@VERSION@/$version/g" -e "s/@BUNDLE_ID@/${HARVEST_BUNDLE_ID:-io.github.banteg.harvest}/g" \
        -e "s/@MINIMUM_MACOS@/$minimum_macos/g" "$packaging/macos/Info.plist" > "$app/Contents/Info.plist"
    plutil -lint -s "$app/Contents/Info.plist"
    cp "$packaging/icons/harvest.icns" "$app/Contents/Resources/harvest.icns"
    printf 'APPL????' > "$app/Contents/PkgInfo"

    local identity="${HARVEST_CODESIGN_IDENTITY:--}"
    local flags=(--force --sign "$identity" --options runtime)
    [ "$identity" = - ] || flags+=(--timestamp)
    codesign "${flags[@]}" "$app"
    codesign --verify --strict --verbose=2 "$app"

    mkdir -p "$out"
    rm -f "$out/$name-macos.zip"
    # ditto keeps the bundle's permissions and signature intact; local extended attributes stay out
    ditto -c -k --norsrc --noextattr --keepParent "$folder" "$out/$name-macos.zip"
    echo "$out/$name-macos.zip"
}

package_windows() {
    local prefix="$work/windows-x86_64"
    build x86_64-windows-gnu "$prefix"
    local folder="$work/$name-windows-x86_64"
    stage "$folder" "$prefix"
    cp "$prefix/bin/harvest.exe" "$folder/harvest.exe"
    mkdir -p "$out"
    zip_folder "$folder" "$out/$name-windows-x86_64.zip"
    echo "$out/$name-windows-x86_64.zip"
}

package_linux() {
    local prefix="$work/linux-x86_64"
    build "x86_64-linux-gnu.$minimum_glibc" "$prefix"
    local folder="$work/$name-linux-x86_64"
    stage "$folder" "$prefix"
    cp "$prefix/bin/harvest" "$folder/harvest"
    cp "$packaging/linux/harvest-port.desktop" "$folder/harvest-port.desktop"
    cp "$packaging/linux/install-desktop-entry.sh" "$folder/install-desktop-entry.sh"
    cp "$packaging/icons/harvest.png" "$folder/harvest-port.png"
    chmod 755 "$folder/harvest" "$folder/install-desktop-entry.sh"
    mkdir -p "$out"
    # files owned by root rather than the builder, and without macOS's extended attributes
    local flags=(--uid 0 --gid 0 --uname root --gname root --no-xattrs --no-mac-metadata)
    if tar --version | grep -q GNU; then flags=(--owner 0 --group 0 --numeric-owner); fi
    tar "${flags[@]}" -C "$work" -czf "$out/$name-linux-x86_64.tar.gz" "$(basename "$folder")"
    echo "$out/$name-linux-x86_64.tar.gz"
}

case "${1:-all}" in
    macos) package_macos ;;
    windows) package_windows ;;
    linux) package_linux ;;
    all)
        if [ "$(uname -s)" = Darwin ]; then package_macos; fi
        package_windows
        package_linux
        ;;
    *) echo "usage: $0 [macos|windows|linux|all]" >&2; exit 2 ;;
esac
