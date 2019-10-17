#!/bin/bash

set -eu

app="$1"
if [ -z "$app" ]; then
	echo "Usage: $0 <appname>"
	echo "Provide appname without the .app extension, please"
	exit 2
fi

frameworks="QtCore QtNetwork QtGui QtXml QtSvg QtWidgets QtPrintSupport QtDBus"

plugins="gif icns ico jpeg tga tiff wbmp webp cocoa minimal offscreen macstyle"

qtdir=$(grep "Command:" Makefile | head -1 | awk '{ print $3; }' | sed s,/bin/.*,,)

if [ ! -d "$qtdir" ]; then
    echo "Failed to discover Qt installation directory from Makefile, exiting"
    exit 2
fi

fdir="$app.app/Contents/Frameworks"
pdir="$app.app/Contents/plugins"

mkdir -p "$fdir"
mkdir -p "$pdir"

echo
echo "Copying frameworks..."
for fwk in $frameworks; do
    if [ ! -d "$qtdir/lib/$fwk.framework" ]; then
	if [ "$fwk" = "QtDBus" ]; then
	    echo "QtDBus.framework not found, assuming Qt was built without DBus support"
	    continue
	fi
    fi
    cp -v "$qtdir/lib/$fwk.framework/$fwk" "$fdir" || exit 2
done

echo "Done"

echo
echo "Copying plugins..."
for plug in $plugins; do
    pfile=$(ls "$qtdir"/plugins/*/libq"$plug".dylib)
    if [ ! -f "$pfile" ]; then
	echo "Failed to find plugin $plug, exiting"
	exit 2
    fi
    target="$pdir"/${pfile##?*plugins/}
    tdir=`dirname "$target"`
    mkdir -p "$tdir"
    cp -v "$pfile" "$target" || exit 2
done

# Sometimes the copied-in files are read-only: correct that
chmod -R u+w "$app.app"

echo "Done"


