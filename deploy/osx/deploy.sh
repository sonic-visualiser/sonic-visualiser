#!/bin/bash

set -e

# Execute this from the top-level directory of the project (the one
# that contains the .app bundle).  Supply the name of the .app bundle
# as argument (the target will use $app.app regardless, but we need
# to know the source)

source="$1"
dmg="$2"
if [ -z "$source" ] || [ ! -d "$source" ] || [ -z "$dmg" ]; then
	echo "Usage: $0 <source.app> <target-dmg-basename>"
	echo "  e.g. $0 MyApplication.app MyApplication"
 	echo "  Version number and .dmg will be appended automatically,"
        echo "  but the .app name must include .app"
	exit 2
fi
app=`basename "$source" .app`

set -u

version=`perl -p -e 's/^[^"]*"([^"]*)".*$/$1/' version.h`
stem=${version%%-*}
stem=${stem%%pre*}
case "$stem" in
    [0-9].[0-9]) bundleVersion="$stem".0 ;;
    [0-9].[0-9].[0-9]) bundleVersion="$stem" ;;
    *) echo "Error: Version stem $stem (of version $version) is neither two- nor three-part number" ;;
esac

echo
echo "Copying in frameworks and plugins from Qt installation directory."

deploy/osx/copy-qt.sh "$app" || exit 2

echo
echo "Fixing up paths."

deploy/osx/paths.sh "$app"

echo
echo "Copying in qt.conf to set local-only plugin paths."
echo "Make sure all necessary Qt plugins are in $source/Contents/plugins/*"
echo "You probably want platforms/, accessible/ and imageformats/ subdirectories."
cp deploy/osx/qt.conf "$source"/Contents/Resources/qt.conf

echo
echo "Copying in plugin load checker."
cp checker/vamp-plugin-load-checker "$source"/Contents/MacOS/

echo
echo "Copying in plugin server."
cp piper-vamp-simple-server "$source"/Contents/MacOS/

echo
echo "Writing version $bundleVersion in to bundle."
echo "(This should be a three-part number: major.minor.point)"

perl -p -e "s/SV_VERSION/$bundleVersion/" deploy/osx/Info.plist \
    > "$source"/Contents/Info.plist

echo "Done: check $source/Contents/Info.plist for sanity please"

echo
echo "Copying in lproj directories containing InfoPlist.strings translation files."
cp -r i18n/*.lproj "$source"/Contents/Resources/

echo
echo "Making target tree."

volume="$app"-"$version"
target="$volume"/"$app".app
dmg="$dmg"-"$version".dmg

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

hdiutil create -srcfolder "$volume" "$dmg" -volname "$volume" -fs HFS+ && 
	rm -r "$volume"

echo
echo "Signing dmg..."

codesign -s "Developer ID Application: Chris Cannam" -fv "$dmg"

echo "Done"
