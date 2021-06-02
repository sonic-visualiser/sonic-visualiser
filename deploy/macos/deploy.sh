#!/bin/bash

set -e

# Execute this from the top-level directory of the project (the one
# that contains the .app bundle).  Supply the name of the application
# as argument.
#
# This now performs *only* the app deployment step - copying in
# libraries and setting up paths etc. It does not create a
# package. Use deploy-and-package.sh for that.

app="$1"
source="$app.app"

if [ -z "$app" ] || [ -n "$2" ]; then
	echo "Usage: $0 <app>"
	echo "  e.g. $0 MyApplication"
 	echo "  The executable must be present in build/<app>."
	echo "  Version number will be extracted from build/version.h."
	exit 2
fi

set -u

mkdir -p "$source/Contents/MacOS"
mkdir -p "$source/Contents/Resources"

cp -a "build/$app" "$source/Contents/MacOS"

version=`perl -p -e 's/^[^"]*"([^"]*)".*$/$1/' build/version.h`
stem=${version%%-*}
stem=${stem%%pre*}
case "$stem" in
    [0-9].[0-9]) bundleVersion="$stem".0 ;;
    [0-9].[0-9].[0-9]) bundleVersion="$stem" ;;
    *) echo "Error: Version stem $stem (of version $version) is neither two- nor three-part number" ;;
esac

echo
echo "Copying in icon."

cp "icons/sv-macicon.icns" "$source/Contents/Resources"


echo
echo "Copying in frameworks and plugins from Qt installation directory."

deploy/macos/copy-qt.sh "$app" || exit 2

echo
echo "Fixing up paths."

deploy/macos/paths.sh "$app"

echo
echo "Copying in qt.conf to set local-only plugin paths."
echo "Make sure all necessary Qt plugins are in $source/Contents/plugins/*"
echo "You probably want platforms/, accessible/ and imageformats/ subdirectories."
cp deploy/macos/qt.conf "$source"/Contents/Resources/qt.conf

echo
echo "Copying in plugin load checker."
cp build/vamp-plugin-load-checker "$source"/Contents/MacOS/

echo
echo "Copying in plugin server."
cp build/piper-vamp-simple-server "$source"/Contents/MacOS/

echo
echo "Copying in piper convert tool."
cp build/piper-convert "$source"/Contents/MacOS/

echo
echo "Copying in lproj directories containing InfoPlist.strings translation files."
cp -r i18n/*.lproj "$source"/Contents/Resources/

echo
echo "Writing version $bundleVersion in to bundle."
echo "(This should be a three-part number: major.minor.point)"

perl -p -e "s/SV_VERSION/$bundleVersion/" deploy/macos/Info.plist \
    > "$source"/Contents/Info.plist

echo "Done: check $source/Contents/Info.plist for sanity please"
