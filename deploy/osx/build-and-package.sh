#!/bin/bash

# This script is very specific to certain Xcode and Qt versions -
# review before running, or do a standard build and a deployment with
# deploy-and-package.sh instead

set -eu

app="Sonic Visualiser"

version=`perl -p -e 's/^[^"]*"([^"]*)".*$/$1/' version.h`

volume="$app"-"$version"
dmg="$volume".dmg
compatdmg="$volume-osx-10.7.dmg"

./repoint install

echo
echo "Rebuilding for current macOS..."
echo

rm -rf .qmake.stash
$HOME/Qt/5.13.1/clang_64/bin/qmake -r
make clean
make -j3
deploy/osx/deploy-and-package.sh

echo
echo "Done, images are in $dmg and $compatdmg"
echo

