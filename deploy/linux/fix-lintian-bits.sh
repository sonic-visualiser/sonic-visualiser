#!/bin/bash

dir=$1

[ -d "$dir" ] || exit 1

strip "$dir"/usr/bin/*

sz=`du -sx --exclude DEBIAN "$dir" | awk '{ print $1; }'`
perl -i -p -e "s/Installed-Size: .*/Installed-Size: $sz/" "$dir"/DEBIAN/control

find "$dir" -name \*~ -exec rm \{\} \;

sudo chown -R root.root "$dir"/*

sudo chmod -R g-w "$dir"/*
