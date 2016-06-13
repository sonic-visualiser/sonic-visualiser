#!/bin/bash

set -eu

tag=`hg tags | grep '^sv_v' | head -1 | awk '{ print $1; }'`

v=`echo "$tag" | sed 's/sv_v//' | sed 's/_.*$//'`

echo
echo -n "Packaging up version $v from tag $tag... "

hg archive -r"$tag" --subrepos --exclude sv-dependency-builds /tmp/sonic-visualiser-"$v".tar.gz

echo Done
echo

# Test that the appropriate version of the docs exist on the website

doc_url="http://sonicvisualiser.org/doc/reference/$v/en/"
doc_status=$(curl -sL -w "%{http_code}" "$doc_url" -o /dev/null)

if [ "$doc_status" = "404" ]; then
    echo "*** WARNING: Documentation URL returns a 404:"
    echo "***          $doc_url"
    echo "***          Please fix this before release!"
    echo
fi
