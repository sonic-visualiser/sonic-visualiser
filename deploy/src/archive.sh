#!/bin/bash

set -eu

tag=`git tag --list --sort=creatordate | grep '^sv_v' | tail -1 | awk '{ print $1; }'`

v=`echo "$tag" | sed 's/sv_v//' | sed 's/_.*$//'`

echo -n "Package up source code for version $v from tag $tag [Yn] ? "
read yn
case "$yn" in "") ;; [Yy]) ;; *) exit 3;; esac
echo "Proceeding"

case $(git status --porcelain --untracked-files=no) in
    "") ;;
    *) echo "ERROR: Current working copy has been modified - unmodified copy required so we can update to tag and back again safely"; exit 2;;
esac

echo
echo -n "Packaging up version $v from tag $tag... "

current=$(git rev-parse --short HEAD)

mkdir -p packages

git checkout "$tag"

./repoint archive "$(pwd)"/packages/sonic-visualiser-"$v".tar.gz --exclude sv-dependency-builds export-tests repoint.pri .gitignore .github .appveyor.yml .hgtags

git checkout "$current"

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
