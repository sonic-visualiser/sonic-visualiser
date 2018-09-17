#!/bin/bash

version=$(perl -p -e 's/^[^"]*"([^"]*)".*$/$1/' version.h | sed 's/-//g')

dockerdir=./deploy/linux/docker
if [ ! -d "$dockerdir" ]; then
    echo "Run this script from the build root"
    exit 2
fi

platform="$1"

if [ -z "$platform" ] || [ -n "$2" ]; then
    echo "Usage: $0 <platform>"
    echo "where <platform> matches the suffix of the Docker file, e.g. ubuntu1604"
    exit 2
fi

set -eu

echo "Building for version $version, platform $platform"
dockerfile="Dockerfile_v${version}_${platform}"

if [ ! -f "$dockerdir/$dockerfile" ]; then
    echo "No matching docker file $dockerfile found in $dockerdir, trying again without version"
    dockerfile="Dockerfile_${platform}"
    if [ ! -f "$dockerdir/$dockerfile" ]; then
        echo "No matching docker file $dockerfile found in $dockerdir either"
        exit 1
    fi
fi

dockertag="cannam/sonic-visualiser-$platform"

sudo docker build -t "$dockertag" -f "$dockerdir/$dockerfile" "$dockerdir"

outdir="$dockerdir/output"
mkdir -p "$outdir"

container=$(sudo docker create "$dockertag")
sudo docker cp "$container":output.tar "$outdir"
sudo docker rm "$container"

( cd "$outdir" ; tar xf output.tar && rm -f output.tar )

echo
echo "Done, output directory contains:"
ls -ltr "$outdir"
