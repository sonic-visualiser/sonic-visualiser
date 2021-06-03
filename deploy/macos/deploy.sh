#!/bin/bash

set -e

# Execute this from the top-level directory of the project (the one
# that contains the .app bundle).  Supply the name of the application
# as argument.
#
# This now performs *only* the app deployment step - copying in
# libraries and setting up paths etc. It does not create a
# package. Use deploy-and-package.sh for that.

usage() {
    echo
    echo "Usage: $0 <app> [<builddir>]"
    echo
    echo "  where <builddir> defaults to \"build\""
    echo 
    echo "For example,"
    echo
    echo "  \$ $0 MyApplication"
    echo "  to deploy app MyApplication built in the directory \"build\""
    echo
    echo "  \$ $0 MyApplication build-release"
    echo "  to deploy app MyApplication built in the directory \"build-release\""
    echo
    echo "The executable must be already present in <builddir>/<app>, and its"
    echo "version number will be extracted from <builddir>/version.h."
    echo 
    echo "In all cases the target will be an app bundle called <app>.app in"
    echo "the current directory."
    echo
    exit 2
}

app="$1"
if [ -z "$app" ]; then
    usage
fi

builddir="$2"
if [ -z "$builddir" ]; then
    builddir=build
elif [ -n "$3" ]; then
    usage
fi

if [ ! -f "$builddir/$app" ]; then
    echo "File $app not found in builddir $builddir"
    exit 2
fi

source="$app.app"

set -u

mkdir -p "$source/Contents/MacOS"
mkdir -p "$source/Contents/Resources"

cp -a "$builddir/$app" "$source/Contents/MacOS"

version=`perl -p -e 's/^[^"]*"([^"]*)".*$/$1/' $builddir/version.h`
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
cp $builddir/vamp-plugin-load-checker "$source"/Contents/MacOS/

echo
echo "Copying in plugin server."
cp $builddir/piper-vamp-simple-server "$source"/Contents/MacOS/

echo
echo "Copying in piper convert tool."
cp $builddir/piper-convert "$source"/Contents/MacOS/

echo
echo "Copying in lproj directories containing InfoPlist.strings translation files."
cp -r i18n/*.lproj "$source"/Contents/Resources/

echo
echo "Writing version $bundleVersion in to bundle."
echo "(This should be a three-part number: major.minor.point)"

perl -p -e "s/SV_VERSION/$bundleVersion/" deploy/macos/Info.plist \
    > "$source"/Contents/Info.plist

echo "Done: check $source/Contents/Info.plist for sanity please"
