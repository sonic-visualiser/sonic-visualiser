#!/bin/bash

set -e

# WARNING: Destructive, makes some assumptions about layout

source="Sonic Visualiser.app"
dmg="Sonic Visualiser"

app=`basename "$source" .app`

set -u

version=`perl -p -e 's/^[^"]*"([^"]*)".*$/$1/' version.h`

rm -rf "$source" "$app-$version" "$app-$version.dmg"

umount "/Volumes/$app-$version"

make
deploy/osx/deploy.sh "$source" "$dmg"

open "$app-$version.dmg"
sleep 5
open "/Volumes/$app-$version/$app.app"

