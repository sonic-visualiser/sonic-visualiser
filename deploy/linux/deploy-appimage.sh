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

for lib in $(ldd "$program" | grep '=> /usr/lib/' | sed 's/^.*=> //' | sed 's/ .*$//'); do
    mkdir -p "$targetdir/$(dirname $lib)"
    cp -L "$lib" "$targetdir/$lib"
done

cp "$program.desktop" "$targetdir/"

cp "icons/sv-icon.svg" "$targetdir/"

cp sv-dependency-builds/linux/appimage/AppRun-x86_64 "$targetdir/AppRun"
chmod +x "$targetdir/AppRun"

ARCH=x86_64 sv-dependency-builds/linux/appimage/appimagetool-x86_64.AppImage "$targetdir"

