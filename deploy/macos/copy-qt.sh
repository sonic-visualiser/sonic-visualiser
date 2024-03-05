#!/bin/bash

set -eu

app="$1"
if [ -z "$app" ]; then
	echo "Usage: $0 <appname>"
	echo "Provide appname without the .app extension, please"
	exit 2
fi

if [ ! -d "$QTDIR" ]; then
    echo "QTDIR must be set"
    exit 2
fi

macdeployqt="$QTDIR/bin/macdeployqt"

if [ ! -x "$macdeployqt" ]; then
    echo "Error: macdeployqt program not found in $macdeployqt"
    exit 1
fi

"$macdeployqt" "$app.app" 

echo "Done"


