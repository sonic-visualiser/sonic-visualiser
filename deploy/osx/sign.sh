#!/bin/bash -x

set -eu

# Execute this from the top-level directory of the project (the one
# that contains the .app bundle).  Supply the name of the .app bundle
# as argument
dir="$1"
if [ -z "$dir" ] || [ ! -d "$dir" ]; then
	echo "Usage: $0 <pkgdir>"
	echo "Where pkgdir is the directory containing <MyApplication>.app"
	echo "All .app bundles in pkgdir will be signed"
	exit 2
fi

# NB at some point we are going to have to include "--options runtime"
# in all of these codesign invocations, and figure out what to do
# about signing plugins...

id="Developer ID Application: Chris Cannam"
opts="-fv --deep --options runtime -i org.sonicvisualiser.SonicVisualiser"
eopts="--entitlements deploy/osx/Entitlements.plist"
hopts="--entitlements deploy/osx/Entitlements-helpers.plist"

for app in "$dir"/*.app; do
    find "$app" -name \*.dylib -print | while read fr; do
	codesign -s "$id" $opts "$fr"
    done
    codesign -s "$id" $opts $hopts "$app/Contents/Resources/vamp-plugin-load-checker"
    codesign -s "$id" $opts $hopts "$app/Contents/Resources/piper-vamp-simple-server"
    codesign -s "$id" $opts $eopts "$app/Contents/MacOS/Sonic Visualiser"
done

