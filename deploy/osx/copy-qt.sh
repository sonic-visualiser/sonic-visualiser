#!/bin/bash

app="$1"
if [ -z "$app" ]; then
	echo "Usage: $0 <appname>"
	echo "Provide appname without the .app extension, please"
	exit 2
fi

frameworks="QtCore QtNetwork QtGui QtXml QtWidgets QtPrintSupport"

plugins="qtaccessiblewidgets qdds qgif qicns qico qjp2 qjpeg qmng qtga qtiff qwbmp qwebp"

platplugins="qcocoa qminimal"

qtdir=$(grep "Command:" Makefile | head -1 | awk '{ print $3; }' | sed s,/bin/.*,,)

if [ ! -d "$qtdir" ]; then
    echo "Failed to discover Qt installation directory from Makefile, exiting"
    exit 2
fi

fdir="$app.app/Contents/Frameworks"
pdir="$app.app/Contents/plugins"
ppdir="$app.app/Contents/plugins/platforms"

mkdir -p "$fdir"
mkdir -p "$pdir"
mkdir -p "$ppdir"

echo
echo "Copying frameworks..."
for fwk in $frameworks; do
    cp -v "$qtdir/lib/$fwk.framework/$fwk" "$fdir" || exit 2
done

echo "Done"

echo
echo "Copying plugins..."
for plug in $plugins; do
    pfile=$(ls "$qtdir"/plugins/*/lib"$plug".dylib)
    if [ ! -f "$pfile" ]; then
	echo "Failed to find plugin $plug, exiting"
	exit 2
    fi
    cp -v "$pfile" "$pdir" || exit 2
done

echo "Done"

echo
echo "Copying platform plugins..."
for plug in $platplugins; do
    pfile=$(ls "$qtdir"/plugins/*/lib"$plug".dylib)
    if [ ! -f "$pfile" ]; then
	echo "Failed to find plugin $plug, exiting"
	exit 2
    fi
    # I really cannot be bothered to figure out why Qt fails if I copy
    # to either one of these alone
    cp -v "$pfile" "$pdir" || exit 2
    cp -v "$pfile" "$ppdir" || exit 2
done

echo "Done"

