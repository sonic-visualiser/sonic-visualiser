#!/bin/bash

usage() {
    echo
    echo "  $0: run clang-tidy on a build with supplied arguments" 1>&2
    echo
    echo "  Run this from the root of the source tree, on a Linux system." 1>&2
    echo "  Assumes that \"./configure && make clean && make\" will produce a successful" 1>&2
    echo "  build using g++." 1>&2
    echo "  All arguments are passed to clang-tidy." 1>&2
    echo
    exit 2
}

if [ t"$1" = "t" ]; then
    usage
fi

ctargs="$@"

echo "clang-tidy args: $ctargs"

set -eu

tmpdir=$(mktemp -d)
trap "rm -rf \$tmpdir\$" 0

#log="build.log"

log="$tmpdir/make.log"
./configure
make clean
make -j3 2>&1 | tee "$log"

list="$tmpdir/tidy.list"

grep '^g[+][+] ' "$log" | grep ' [-]c ' > "$list"

cat "$list" | while read line ; do

    filename=${line##* }
    remainder=${line% *}
    cc=${remainder%% *}
    ccargs=${remainder#* }

    ccargs=$(echo "$ccargs" | sed 's/-flto //')

    echo
    
    case "$filename" in
        bq*) echo "ignoring $filename" ;;
        o/*) echo "ignoring $filename" ;;
        vamp-*) echo "ignoring $filename" ;;
        dataquay/*) echo "ignoring $filename" ;;
        src/*) echo "ignoring $filename" ;;
        *) echo "not ignoring $filename"
           echo clang-tidy $ctargs "$filename" -- $ccargs ;
           clang-tidy $ctargs "$filename" -- $ccargs ;;
    esac
done

