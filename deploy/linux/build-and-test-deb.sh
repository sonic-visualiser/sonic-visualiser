#!/bin/bash
#
# Docker required

set -eu

case $(git status --porcelain --untracked-files=no) in
    "") ;;
    *) echo "ERROR: Current working copy has been modified - unmodified copy required so we know we can check it out separately and obtain the same contents"; exit 2;;
esac

current=$(git rev-parse --short HEAD)

version=$(grep '^ *version:' meson.build | sed "s/^ *version: *'//" | sed "s/'.*$//")

echo
echo "Building Debian deb archive for version $version from revision $current..."

dockerdir=deploy/linux/docker

cat "$dockerdir"/Dockerfile_deb.in | \
    perl -p -e "s/\[\[REVISION\]\]/$current/g" | \
    perl -p -e "s/\[\[RELEASE\]\]/$version/g" > \
         "$dockerdir"/Dockerfile_deb.gen

cat "$dockerdir"/Dockerfile_test_deb.in | \
    perl -p -e "s/\[\[REVISION\]\]/$current/g" | \
    perl -p -e "s/\[\[RELEASE\]\]/$version/g" > \
         "$dockerdir"/Dockerfile_test_deb.gen

fgrep 'hg.sr.ht' ~/.ssh/known_hosts > "$dockerdir"/known_hosts
cp ~/.ssh/id_rsa_build "$dockerdir"/id_rsa_build
chmod 600 "$dockerdir"/known_hosts "$dockerdir"/id_rsa_build
trap "rm $dockerdir/known_hosts $dockerdir/id_rsa_build" 0

dockertag="cannam/sonic-visualiser-deb-$current"

sudo docker build -t "$dockertag" -f "$dockerdir"/Dockerfile_deb.gen "$dockerdir"

outdir="$dockerdir/output"
mkdir -p "$outdir"

container=$(sudo docker create "$dockertag")

sudo docker cp "$container":output-deb.tar "$outdir"
sudo docker rm "$container"

( cd "$outdir" ; tar xf output-deb.tar && rm -f output-deb.tar )

sudo docker build -f "$dockerdir"/Dockerfile_test_deb.gen "$dockerdir"
