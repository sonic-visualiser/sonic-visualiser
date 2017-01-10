#!/bin/bash

set -e

app="$1"
if [ -z "$app" ]; then
	echo "Usage: $0 <appname>"
	echo "Provide appname without the .app extension, please"
	exit 2
fi

set -u

frameworks="QtCore QtNetwork QtGui QtXml QtSvg QtWidgets QtPrintSupport QtDBus"

echo
echo "I expect you to have already copied these frameworks from the Qt installation to"
echo "$app.app/Contents/Frameworks -- expect errors to follow if they're missing:"
echo "$frameworks"
echo

echo "Fixing up loader paths in binaries..."

for fwk in $frameworks; do
    install_name_tool -id $fwk "$app.app/Contents/Frameworks/$fwk"
done

find "$app.app" -name \*.dylib -print | while read x; do
    install_name_tool -id "`basename \"$x\"`" "$x"
done

for fwk in $frameworks; do
    find "$app.app" -type f -print | while read x; do
	if [ -x "$x" ]; then
            current=$(otool -L "$x" | grep "$fwk" | grep amework | grep -v ':$' | awk '{ print $1; }')
            [ -z "$current" ] && continue
            echo "$x has $current"
            relative=$(echo "$x" | sed -e "s,$app.app/Contents/,," \
				       -e 's,[^/]*/,../,g' \
				       -e 's,/[^/]*$,/Frameworks/'"$fwk"',' )
            echo "replacing with relative path $relative"
            install_name_tool -change "$current" "@loader_path/$relative" "$x"
	fi
    done
done

find "$app.app" -type f -print | while read x; do
    if [ -x "$x" ]; then
	qtdep=$(otool -L "$x" | grep Qt | grep amework | grep -v ':$' | grep -v '@loader_path' | awk '{ print $1; }')
	if [ -n "$qtdep" ]; then
	    echo
	    echo "ERROR: File $x depends on Qt framework(s) not apparently present in the bundle:"
	    echo $qtdep
	    exit 1
	fi
    fi
done

echo "Done: be sure to run the app and see that it works!"


