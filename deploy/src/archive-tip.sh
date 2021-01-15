#!/bin/bash

case $(git status --porcelain --untracked-files=no) in
    "") ;;
    *) echo "ERROR: Current working copy has been modified - unmodified copy required"; exit 2;;
esac

id=$(git rev-parse --short HEAD)

echo "Packaging from id $id..."

./repoint archive /tmp/sonic-visualiser-"$id".tar.gz --exclude sv-dependency-builds export-tests repoint.pri

echo Done
echo

