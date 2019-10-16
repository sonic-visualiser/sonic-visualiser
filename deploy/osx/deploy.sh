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

if [ -z "$app" ] || [ ! -d "$source" ] || [ -n "$2" ]; then
	echo "Usage: $0 <app>"
	echo "  e.g. $0 MyApplication"
 	echo "  The app bundle must exist in ./<app>.app."
	echo "  Version number will be extracted from version.h."
	exit 2
fi

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
echo "Copying in lproj directories containing InfoPlist.strings translation files."
cp -r i18n/*.lproj "$source"/Contents/Resources/

echo
echo "Writing version $bundleVersion in to bundle."
echo "(This should be a three-part number: major.minor.point)"

perl -p -e "s/SV_VERSION/$bundleVersion/" deploy/osx/Info.plist \
    > "$source"/Contents/Info.plist

echo "Done: check $source/Contents/Info.plist for sanity please"
