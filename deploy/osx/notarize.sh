#!/bin/bash

## The following assumes we have generated an app password at
## appleid.apple.com and then stored it to keychain id "altool" using
## e.g.
## security add-generic-password -a "cannam+apple@all-day-breakfast.com" \
##   -w "generated-app-password" -s "altool"

## NB to verify:
# spctl -a -v "/Applications/Application.app"

user="cannam+apple@all-day-breakfast.com"
bundleid="org.sonicvisualiser.SonicVisualiser"

set -e

dmg="$1"

if [ ! -f "$dmg" ] || [ -n "$2" ]; then
    echo "Usage: $0 <dmg>"
    echo "  e.g. $0 MyApplication-1.0.dmg"
    exit 2
fi

set -u

echo
echo "Uploading for notarization..."

uuidfile=.notarization-uuid
rm -f "$uuidfile"

xcrun altool --notarize-app \
    -f "$dmg" \
    --primary-bundle-id "$bundleid" \
    -u "$user" \
    -p @keychain:altool 2>&1 | tee "$uuidfile"

uuid=$(cat "$uuidfile" | grep RequestUUID | awk '{ print $3; }')

if [ -z "$uuid" ]; then
    echo
    echo "Failed (no UUID returned, check output)"
    exit 1
fi

echo "Done, UUID is $uuid"

echo
echo "Waiting and checking for completion..."

while true ; do
    sleep 30
    status=$(xcrun altool --notarization-info "$uuid" -u "$user" -p @keychain:altool 2>&1)
    if echo "$status" | grep -q 'Package Approved' ; then
	echo
	echo "Approved! Status output is:"
	echo "$status"
	break
    elif echo "$status" | grep -q 'in progress' ; then
	echo
	echo "Still in progress... Status output is:"
	echo "$status"
	echo "Waiting..."
    else 
	echo
	echo "Failure or unknown status in output:"
	echo "$status"
	exit 2
    fi
done

echo
echo "Stapling to package..."

xcrun stapler staple "$dmg" || exit 1

