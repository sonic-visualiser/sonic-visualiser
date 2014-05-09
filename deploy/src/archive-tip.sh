#!/bin/bash

tag=`hg id | awk '{ print $1; }'`

echo "Packaging from tag $tag..."

hg archive -r"$tag" --subrepos --exclude sv-dependency-builds /tmp/sonic-visualiser-"$tag".tar.gz

