#!/bin/bash

set -eu

app="$1"
if [ -z "$app" ]; then
	echo "Usage: $0 <appname>"
	echo "Provide appname without the .app extension, please"
	exit 2
fi

frameworks="QtCore QtNetwork QtGui QtXml QtSvg QtTest QtPdf QtWidgets QtPrintSupport QtDBus"

plugins="gif ico jpeg pdf svg cocoa minimal offscreen macstyle"

qtdir="$QTDIR"

if [ ! -d "$qtdir" ]; then
    echo "QTDIR must be set"
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

psrcdir="$qtdir"/plugins/
if [ ! -d "$psrcdir" ]; then
    echo "Failed to find plugin dir $psrcdir, trying alternative location"
    psrcdir="$qtdir"/share/qt/plugins/
    if [ ! -d "$psrcdir" ]; then
        echo "Failed to find plugin dir alternative $psrcdir, exiting"
        exit 2
    fi
fi
        
for plug in $plugins; do
    pfile=$(ls "$psrcdir"/*/libq"$plug".dylib)
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


