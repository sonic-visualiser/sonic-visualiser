#!/bin/bash

set -eu

program=sonic-visualiser
checker=vamp-plugin-load-checker
piper=piper-vamp-simple-server

targetdir="${program}.AppDir"

echo "Target dir is $targetdir"

if [ -d "$targetdir" ]; then
    echo "Target directory exists, not overwriting"
    exit
fi

mkdir "$targetdir"

mkdir -p "$targetdir"/usr/bin
mkdir -p "$targetdir"/usr/lib

cp "$program" "$checker" "$piper" "$targetdir"/usr/bin/

ldd /usr/lib/x86_64-linux-gnu/libpulse.so.0 || true

add_dependencies() {
    local binary="$1"
    for lib in $(ldd "$binary" | egrep '=> (/usr)?/lib/' | sed 's/^.*=> //' | sed 's/ .*$//' | grep -v 'libc.so' | grep -v 'libm.so'); do
        mkdir -p "$targetdir/$(dirname $lib)"
        if [ ! -f "$targetdir/$lib" ]; then
            cp -Lv "$lib" "$targetdir/$lib"
            chmod +x "$targetdir/$lib"
            add_dependencies "$lib"
        fi
    done
}

add_dependencies "$program"

cp "$program.desktop" "$targetdir/"

cp "icons/sv-icon.svg" "$targetdir/"

cp sv-dependency-builds/linux/appimage/AppRun-x86_64 "$targetdir/AppRun"
chmod +x "$targetdir/AppRun"

ARCH=x86_64 sv-dependency-builds/linux/appimage/appimagetool-x86_64.AppImage "$targetdir"

