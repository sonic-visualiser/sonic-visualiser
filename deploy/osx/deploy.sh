#!/bin/bash

# Execute this from the top-level directory of the project (the one
# that contains the .app bundle).  Supply the name of the .app bundle
# as argument (the target will use $app.app regardless, but we need
# to know the source)

source="$1"
dmg="$2"
if [ -z "$source" ] || [ ! -d "$source" ] || [ -z "$dmg" ]; then
	echo "Usage: $0 <source.app> <target-dmg-basename>"
	exit 2
fi
app=`basename "$source" .app`

version=`perl -p -e 's/^[^"]*"([^"]*)".*$/$1/' version.h`
case "$version" in
    [0-9].[0-9]) bundleVersion="$version".0 ;;
    [0-9].[0-9].[0-9]) bundleVersion="$version" ;;
    *) echo "Error: Version $version is neither two- nor three-part number" ;;
esac

echo
echo "Making target tree."

volume="$app"-"$version"
target="$volume"/"$app".app
dmg="$dmg"-"$version".dmg

mkdir "$volume" || exit 1

ln -s /Applications "$volume"/Applications
cp README README.OSC COPYING CHANGELOG "$volume/"
cp -rp "$source" "$target"

echo "Done"

echo "Writing version $bundleVersion in to bundle."
echo "(This should be a three-part number: major.minor.point)"

perl -p -e "s/SV_VERSION/$bundleVersion/" deploy/osx/Info.plist \
    > "$target"/Contents/Info.plist

echo "Done: check $target/Contents/Info.plist for sanity please"

echo
echo "Making dmg..."

hdiutil create -srcfolder "$volume" "$dmg" -volname "$volume" && 
	rm -r "$volume"

echo "Done"
