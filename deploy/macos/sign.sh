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

entitlements=deploy/macos/Entitlements.plist
helper_entitlements=deploy/macos/HelperEntitlements.plist

gatekeeper_key="Developer ID Application: Particular Programs Ltd (73F996B92S)"

for app in "$dir"/*.app; do
    find "$app" -name \*.dylib -print | while read fr; do
	codesign -s "$gatekeeper_key" -fv --deep --options runtime "$fr"
    done
    find "$app/Contents/MacOS" -type f -print | while read f; do
	codesign -s "$gatekeeper_key" -fv --deep --options runtime --entitlements "$entitlements" "$f"
    done
    find "$app/Contents/Frameworks" -type f -print | while read f; do
	codesign -s "$gatekeeper_key" -fv --deep --options runtime --entitlements "$entitlements" "$f"
    done
    codesign -s "$gatekeeper_key" -fv --deep --options runtime --entitlements "$entitlements" "$app"
done

