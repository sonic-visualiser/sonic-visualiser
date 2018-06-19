#!/bin/bash

id=`hg id | awk '{ print $1; }'`

case "$id" in
    *+) echo "ERROR: Current working copy has been modified - unmodified copy required"; exit 2;;
    *);;
esac

echo "Packaging from id $id..."

hg update -r"$id"

./repoint archive /tmp/sonic-visualiser-"$id".tar.gz --exclude sv-dependency-builds repoint.pri

echo Done
echo

