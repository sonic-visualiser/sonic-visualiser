#!/bin/bash

# This is just a scrapbook for the mo

## Before this, we need to open Application Loader and log in to the
## right iTunes Connect account

## Looks like the workflow has changed to using app-specific
## passwords, for 2FA reasons.  See
## https://developer.apple.com/documentation/xcode/notarizing_your_app_before_distribution/customizing_the_notarization_workflow?language=objc

## The following assumes we have generated an app password at
## appleid.apple.com and then stored it to keychain id "altool" using
## e.g.
## security add-generic-password -a "cannam+apple@all-day-breakfast.com" \
##   -w "generated-app-password" -s "altool"

## todo: script this

# xcrun altool --notarize-app -f "Sonic Visualiser-4.0-pre2.dmg" --primary-bundle-id org.sonicvisualiser.SonicVisualiser -u "cannam+apple@all-day-breakfast.com" -p @keychain:altool

## That churns for a while and then dumps out a UUID

# xcrun altool --notarization-info UUID -u "cannam+apple@all-day-breakfast.com" -p @keychain:altool

## Returns "in progress" at first, then eventually a failure report
## with a URL that can be retrieved as JSON payload using wget. An
## email is also sent to the iTunes Connect account holder when it
## completes

# xcrun stapler staple -v "Sonic Visualiser-3.2.dmg"

# spctl -a -v "/Applications/Sonic Visualiser.app"



