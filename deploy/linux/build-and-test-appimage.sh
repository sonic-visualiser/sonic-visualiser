#!/bin/bash
#
# Docker required

set -eu

current=$(hg id | awk '{ print $1; }')

case "$current" in
    *+) echo "ERROR: Current working copy has been modified - unmodified copy required so we know we can check it out separately and obtain the same contents"; exit 2;;
    *);;
esac

echo
echo -n "Building appimage from revision $current..."

dockerdir=deploy/linux/docker

cat "$dockerdir"/Dockerfile_appimage.in | \
    perl -p -e "s/\[\[REVISION\]\]/$current/g" > \
         "$dockerdir"/Dockerfile_appimage.gen

cat "$dockerdir"/Dockerfile_test_appimage.in | \
    perl -p -e "s/\[\[REVISION\]\]/$current/g" > \
         "$dockerdir"/Dockerfile_test_appimage.gen

dockertag="cannam/sonic-visualiser-appimage-$current"

sudo docker build -t "$dockertag" -f "$dockerdir"/Dockerfile_appimage.gen "$dockerdir"

outdir="$dockerdir/output"
mkdir -p "$outdir"

container=$(sudo docker create "$dockertag")

sudo docker cp "$container":output.tar "$outdir"
sudo docker rm "$container"

( cd "$outdir" ; tar xf output.tar && rm -f output.tar )

sudo docker build -f "$dockerdir"/Dockerfile_test_appimage.gen "$dockerdir"
