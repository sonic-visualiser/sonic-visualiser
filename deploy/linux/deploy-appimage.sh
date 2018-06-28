#!/bin/bash

set -eu

program=sonic-visualiser
checker=vamp-plugin-load-checker
piper=piper-vamp-simple-server

get_id() {
    if [ -d .hg ]; then
        hg id | sed 's/[+ ].*$//'
    elif [ -d .git ]; then
        git rev-parse --short HEAD
    else
        echo "WARNING: can't figure out revision from VCS metadata" 1>&2
        echo "unknown"
    fi
}

version=$(get_id)

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

    for lib in $(ldd "$binary" | egrep '=> (/usr)?(/local)?/lib/' | \
                     sed 's/^.*=> //' | sed 's/ .*$//'); do

        base=$(basename "$lib")
        if grep -v '^#' sv-dependency-builds/linux/appimage/excludelist |
                grep -q "^$base$" ; then
            echo "excluding: $lib"
            continue
        fi
        
        mkdir -p "$targetdir/$(dirname $lib)"

        if [ ! -f "$targetdir/$lib" ]; then

            cp -Lv "$lib" "$targetdir/$lib"
            chmod +x "$targetdir/$lib"

            # copy e.g. /usr/lib/pulseaudio/libpulsecommon-*.so up a
            # level to something in the load path
            last_element=$(basename $(dirname "$lib"))
            case "$last_element" in
                lib) ;;
                *-gnu) ;;
                *) cp -v "$targetdir/$lib" "$targetdir/$(dirname $(dirname $lib))"
            esac
            
            add_dependencies "$lib"
            
        fi
    done
}

add_dependencies "$program"

cp -v "$targetdir/usr/local/lib/"* "$targetdir/usr/lib/"

cp "$program.desktop" "$targetdir/"

cp "icons/sv-icon.svg" "$targetdir/"

cp sv-dependency-builds/linux/appimage/AppRun-x86_64 "$targetdir/AppRun"
chmod +x "$targetdir/AppRun"

ARCH=x86_64 sv-dependency-builds/linux/appimage/appimagetool-x86_64.AppImage "$targetdir" "SonicVisualiser-$version-x86_64.AppImage"

