#!/bin/bash

set -eu

app="Sonic Visualiser"

version=`perl -p -e 's/^[^"]*"([^"]*)".*$/$1/' version.h`

source="$app.app"
volume="$app"-"$version"
target="$volume"/"$app".app
dmg="$app"-"$version".dmg

if [ -d "$volume" ]; then
    echo "Target directory $volume already exists, not overwriting"
    exit 2
fi

if [ -f "$dmg" ]; then
    echo "Target disc image $dmg already exists, not overwriting"
    exit 2
fi

echo
echo "(Re-)running deploy script..."

deploy/osx/deploy.sh "$app" || exit 1

echo
echo "Making target tree."

mkdir "$volume" || exit 1

ln -s /Applications "$volume"/Applications
cp README.md "$volume/README.txt"
cp README.OSC "$volume/README-OSC.txt"
cp COPYING "$volume/COPYING.txt"
cp CHANGELOG "$volume/CHANGELOG.txt"
cp CITATION "$volume/CITATION.txt"
cp -rp "$source" "$target"

# update file timestamps so as to make the build date apparent
find "$volume" -exec touch \{\} \;

echo "Done"

echo
echo "Code-signing volume..."

deploy/osx/sign.sh "$volume" || exit 1

echo "Done"

echo
echo "Making dmg..."

rm -f "$dmg"

hdiutil create -srcfolder "$volume" "$dmg" -volname "$volume" -fs HFS+ && 
	rm -r "$volume"

echo "Done"

echo
echo "Signing dmg..."

codesign -s "Developer ID Application: Chris Cannam" -fv "$dmg"

echo
echo "Submitting dmg for notarization..."

deploy/osx/notarize.sh "$dmg" || exit 1

echo "Done"
