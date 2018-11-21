#!/bin/bash
#
# Docker required

set -eu

current=$(hg id | awk '{ print $1; }')
release=$(perl -p -e 's/^[^"]*"([^"]*)".*$/$1/' version.h)

case "$current" in
    *+) echo "ERROR: Current working copy has been modified - unmodified copy required so we know we can check it out separately and obtain the same contents"; exit 2;;
    *);;
esac

echo
echo "Building Debian deb archive from revision $current..."

dockerdir=deploy/linux/docker

cat "$dockerdir"/Dockerfile_deb.in | \
    perl -p -e "s/\[\[REVISION\]\]/$current/g" | \
    perl -p -e "s/\[\[RELEASE\]\]/$release/g" > \
         "$dockerdir"/Dockerfile_deb.gen

cat "$dockerdir"/Dockerfile_test_deb.in | \
    perl -p -e "s/\[\[REVISION\]\]/$current/g" | \
    perl -p -e "s/\[\[RELEASE\]\]/$release/g" > \
         "$dockerdir"/Dockerfile_test_deb.gen

dockertag="cannam/sonic-visualiser-deb-$current"

sudo docker build -t "$dockertag" -f "$dockerdir"/Dockerfile_deb.gen "$dockerdir"

outdir="$dockerdir/output"
mkdir -p "$outdir"

container=$(sudo docker create "$dockertag")

sudo docker cp "$container":output-deb.tar "$outdir"
sudo docker rm "$container"

( cd "$outdir" ; tar xf output-deb.tar && rm -f output-deb.tar )

sudo docker build -f "$dockerdir"/Dockerfile_test_deb.gen "$dockerdir"
