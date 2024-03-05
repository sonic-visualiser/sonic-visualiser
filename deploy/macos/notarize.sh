#!/bin/bash

## NB to verify:
# spctl -a -v "/Applications/Application.app"

user="appstore@particularprograms.co.uk"
team_id="73F996B92S"

set -eu

. deploy/metadata.sh

bundleid="$full_ident"
dmg="$full_dmg"

echo
echo "Uploading for notarization..."

xcrun notarytool submit \
    "$dmg" \
    --apple-id "$user" \
    --team-id "$team_id" \
    --keychain-profile notarytool-cannam \
    --wait --progress

echo
echo "Stapling to package..."

xcrun stapler staple "$dmg" || exit 1

