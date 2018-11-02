#!/bin/bash

# This is just a scrapbook for the mo

## Before this, we need to open Application Loader and log in to the
## right iTunes Connect account

# xcrun altool --notarize-app -f "Sonic Visualiser-3.2.dmg" --primary-bundle-id org.sonicvisualiser.SonicVisualiser -u "cannam+apple@all-day-breakfast.com" -p @keychain:"Application Loader: cannam+apple@all-day-breakfast.com"

## That churns for a while and then dumps out a UUID

# xcrun altool --notarization-info UUID -u "cannam+apple@all-day-breakfast.com" -p @keychain:"Application Loader: cannam+apple@all-day-breakfast.com"

## Returns "in progress" at first, then eventually a failure report
## with a URL that can be retrieved as JSON payload using wget. An
## email is also sent to the iTunes Connect account holder when it
## completes

# xcrun stapler staple -v "Sonic Visualiser-3.2.dmg"

# spctl -a -v "/Applications/Sonic Visualiser.app"



