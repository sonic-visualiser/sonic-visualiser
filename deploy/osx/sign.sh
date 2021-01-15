#!/bin/bash 

set -e

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

set -u

entitlements=deploy/osx/Entitlements.plist
helper_entitlements=deploy/osx/HelperEntitlements.plist

gatekeeper_key="Developer ID Application: Particular Programs Ltd (73F996B92S)"

for app in "$dir"/*.app; do
    find "$app" -name \*.dylib -print | while read fr; do
	codesign -s "$gatekeeper_key" -fv --deep --options runtime "$fr"
    done
    codesign -s "$gatekeeper_key" -fv --deep --options runtime --entitlements "$entitlements" "$app/Contents/MacOS/Sonic Visualiser"
    codesign -s "$gatekeeper_key" -fv --deep --options runtime --entitlements "$helper_entitlements" "$app/Contents/MacOS/vamp-plugin-load-checker"
    codesign -s "$gatekeeper_key" -fv --deep --options runtime --entitlements "$helper_entitlements" "$app/Contents/MacOS/piper-vamp-simple-server"
    codesign -s "$gatekeeper_key" -fv --deep --options runtime --entitlements "$entitlements" "$app"
done

