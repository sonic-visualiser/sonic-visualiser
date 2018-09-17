#!/bin/bash 

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
for app in "$dir"/*.app; do
    codesign -s "Developer ID Application: Chris Cannam" -fv --deep "$app"
#    find "$app" -name Qt\* -print | while read fr; do
#	codesign -s "Developer ID Application: Chris Cannam" -fv "$fr"
#    done
#    find "$app" -name \*.dylib -print | while read fr; do
#	codesign -s "Developer ID Application: Chris Cannam" -fv "$fr"
#    done
#    find "$app/Contents/MacOS" -type f -print | while read fr; do
#	codesign -s "Developer ID Application: Chris Cannam" -fv "$fr"
#    done
    codesign -s "Developer ID Application: Chris Cannam" -fv \
         --requirements '=designated =>  identifier "org.sonicvisualiser.SonicVisualiser" and ( (anchor apple generic and    certificate leaf[field.1.2.840.113635.100.6.1.9] ) or (anchor apple generic and    certificate 1[field.1.2.840.113635.100.6.2.6]  and    certificate leaf[field.1.2.840.113635.100.6.1.13] and    certificate leaf[subject.OU] = "M2H8666U82"))' \
         "$app"
done

