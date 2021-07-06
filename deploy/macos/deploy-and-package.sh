#!/bin/bash

# Run this from the project root, without arguments, or with the
# single argument --no-notarization to skip the notarize step

set -e

##!!! Make this accept more than one builddir for different architectures - lipo together the SV and library binaries, and include the helpers from the first builddir as primary and second, if present, as -translated suffix

usage() {
    echo
    echo "Usage: $0 [--no-notarization] [<builddir>]"
    echo
    echo "  where <builddir> defaults to \"build\""
    echo
    exit 2
}

notarize=yes
if [ "$1" = "--no-notarization" ]; then
    notarize=no
    shift
fi

app="Sonic Visualiser"

builddir="$1"
if [ -z "$builddir" ]; then
    builddir=build
else
    shift
fi

if [ -n "$1" ]; then
    usage
fi

if [ ! -f "$builddir/$app" ]; then
    echo "File $app not found in builddir $builddir"
    exit 2
fi

set -u

version=`perl -p -e 's/^[^"]*"([^"]*)".*$/$1/' $builddir/version.h`

source="$app.app"
volume="$app"-"$version"
target="$volume"/"$app".app
dmg="$volume".dmg

if [ -d "$volume" ]; then
    echo "Target directory $volume already exists, not overwriting"
    exit 2
fi

if [ -f "$dmg" ]; then
    echo "Target disc image $dmg already exists, not overwriting"
    exit 2
fi

if [ "$notarize" = no ]; then
    echo
    echo "Note: The --no-notarization flag is set: won't be submitting for notarization"
fi

echo
echo "(Re-)running deploy script..."

deploy/macos/deploy.sh "$app" "$builddir" || exit 1

echo
echo "Making target tree."

mkdir "$volume" || exit 1

##!!! This is the point where we should lipo together the architectures

ln -s /Applications "$volume"/Applications
cp README.md "$volume/README.txt"
cp README_OSC.md "$volume/README_OSC.txt"
cp COPYING "$volume/COPYING.txt"
cp CHANGELOG "$volume/CHANGELOG.txt"
cp CITATION "$volume/CITATION.txt"
cp -rp "$source" "$target"

# update file timestamps so as to make the build date apparent
find "$volume" -exec touch \{\} \;

echo "Done"

echo
echo "Code-signing volume..."

deploy/macos/sign.sh "$volume" || exit 1

echo "Done"

echo
echo "Making dmg..."

rm -f "$dmg"

hdiutil create -srcfolder "$volume" "$dmg" -volname "$volume" -fs HFS+ && 
	rm -r "$volume"

echo "Done"

echo
echo "Signing dmg..."

gatekeeper_key="Developer ID Application: Particular Programs Ltd (73F996B92S)"

codesign -s "$gatekeeper_key" -fv "$dmg"

if [ "$notarize" = no ]; then
    echo
    echo "The --no-notarization flag was set: not submitting for notarization"
else
    echo
    echo "Submitting dmg for notarization..."

    deploy/macos/notarize.sh "$dmg" || exit 1
fi

echo "Done"
