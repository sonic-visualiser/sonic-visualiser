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

# NB at some point we are going to have to include "--options runtime"
# in all of these codesign invocations, and figure out what to do
# about signing plugins...

for app in "$dir"/*.app; do
    find "$app" -name \*.dylib -print | while read fr; do
	codesign -s "Developer ID Application: Chris Cannam" -fv --deep "$fr"
    done
    codesign -s "Developer ID Application: Chris Cannam" -fv --deep "$app/Contents/MacOS/Sonic Visualiser"
    codesign -s "Developer ID Application: Chris Cannam" -fv --deep "$app"
#    codesign -s "Developer ID Application: Chris Cannam" -fv --deep \
#         --requirements '=designated =>  identifier "org.sonicvisualiser.SonicVisualiser" and ( (anchor apple generic and    certificate leaf[field.1.2.840.113635.100.6.1.9] ) or (anchor apple generic and    certificate 1[field.1.2.840.113635.100.6.2.6]  and    certificate leaf[field.1.2.840.113635.100.6.1.13] and    certificate leaf[subject.OU] = "M2H8666U82"))' \
#         "$app"
done

