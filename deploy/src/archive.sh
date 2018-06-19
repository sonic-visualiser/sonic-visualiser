#!/bin/bash

set -eu

tag=`hg tags | grep '^sv_v' | head -1 | awk '{ print $1; }'`

v=`echo "$tag" | sed 's/sv_v//' | sed 's/_.*$//'`

current=$(hg id | awk '{ print $1; }')

case "$current" in
    *+) echo "ERROR: Current working copy has been modified - unmodified copy required so we can update to tag and back again safely"; exit 2;;
    *);;
esac
          
echo
echo -n "Packaging up version $v from tag $tag... "

hg update -r"$tag"

./repoint archive /tmp/sonic-visualiser-"$v".tar.gz --exclude sv-dependency-builds repoint.pri

hg update -r"$current"

echo Done
echo

# Test that the appropriate version of the docs exist on the website

doc_url="http://sonicvisualiser.org/doc/reference/$v/en/"
doc_status=$(curl -sL -w "%{http_code}" "$doc_url" -o /dev/null)

if [ "$doc_status" = "404" ]; then
    echo "*** WARNING: Documentation URL returns a 404:"
    echo "***          $doc_url"
    echo "***          Please fix this before release!"
    echo "***          And remember to update the link from"
    echo "             http://www.sonicvisualiser.org/documentation.html !"
    echo
fi
