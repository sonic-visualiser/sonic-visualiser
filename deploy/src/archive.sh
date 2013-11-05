#!/bin/bash

tag=`hg tags | grep '^sv_v' | head -1 | awk '{ print $1; }'`

v=`echo "$tag" |sed 's/sv_v//'`

echo "Packaging up version $v from tag $tag..."

hg archive -r"$tag" --subrepos --exclude sv-dependency-builds /tmp/sonic-visualiser-"$v".tar.gz

