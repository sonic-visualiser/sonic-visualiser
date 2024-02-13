#!/bin/bash

set -e

if [ -n "$1" ]; then
    echo "Usage: $0" 1>&2
    exit 2
fi

set -u

sv="../build/sonic-visualiser"
if [ ! -f "$sv" -o ! -x "$sv" ]; then
    echo "This script must be run from the sonic-visualiser/export-tests directory" 1>&2
    echo "It expects a sonic-visualiser binary to be found in ../build/" 1>&2
    exit 1
fi

version=$("$sv" -v 2>&1 | grep -v App)
adequate=no
case "$version" in
    [0123].*) ;;
    4.0*) ;;
    [1-9]*) adequate=yes ;;
    *) echo "Failed to query Sonic Visualiser version" 1>&2
       exit 1 ;;
esac
if [ "$adequate" = "no" ]; then
    echo "Sonic Visualiser version must be at least 4.1 (supporting --osc-script option with standard input)" 1>&2
    exit 1
fi

tmpdir=$(mktemp -d)
trap "rm -rf $tmpdir" 0

failure=no

for method in 1 2; do

    echo
    echo "Testing method $method..."

    actual="$tmpdir/3dplot.csv"
    expected="layers-expected/3dplot.csv"

    rm -f "$actual"
    
    if [ "$method" = "1" ]; then
        ( echo "/open s1.wav" ;
          echo "/transform vamp:qm-vamp-plugins:qm-keydetector:keystrength" ;
          echo "/exportlayer $actual" ;
          echo "/quit" ) |
            "$sv" --osc-script -
    else
        ( echo "/transform vamp:qm-vamp-plugins:qm-keydetector:keystrength" ;
          echo "/exportlayer $actual" ;
          echo "/quit" ) |
            "$sv" --osc-script - s1.wav
    fi

    if ! cmp -s "$actual" "$expected" ; then
        echo
        echo "Test failed for method $method"
        echo
        echo "Actual:"
        ls -l "$actual"
        echo "Expected:"
        ls -l "$expected"
        echo
        echo "Diff begins:"
        git diff --no-index --word-diff=color --word-diff-regex=. "$actual" "$expected" | head
        echo
        failure=yes
    fi
done

if [ "$failure" == "no" ]; then
    echo
    echo Passed
fi
