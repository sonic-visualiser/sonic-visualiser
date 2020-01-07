#!/bin/bash
#
# Regression tests for layer export to CSV
# Must be run from directory that contains this script

# NB hardcoded assumptions here about the contents of the session
# file, so the session file (all.sv) is also hardcoded. The session is
# expected to consist of:
# 
# - pane 1 with 3 layers (ruler, waveform, instants)
# - pane 2 with 3 layers (ruler, waveform, values)
# - pane 3 with 3 layers (ruler, image, regions)
# - pane 4 with 3 layers (ruler, text, notes)
# - pane 5 with 2 layers (ruler, 3d plot)
# - pane 6 with 3 layers (ruler, spectrogram, boxes)
# - pane 7 with 2 layers (ruler, peak frequency spectrogram)

set -e

if [ -n "$1" ]; then
    echo "Usage: $0" 1>&2
    exit 2
fi

set -u

sv="../sonic-visualiser"
if [ ! -f "$sv" -o ! -x "$sv" ]; then
    echo "This script must be run from the sonic-visualiser/test directory" 1>&2
    exit 1
fi

version=$("$sv" -v 2>&1)
adequate=no
case "$version" in
    [012].*) ;;
    3.[012]) ;;
    3.[012].*) ;;
    [1-9]*) adequate=yes ;;
    *) echo "Failed to query Sonic Visualiser version" 1>&2
       exit 1 ;;
esac
if [ "$adequate" = "no" ]; then
    echo "Sonic Visualiser version must be at least 3.3 (supporting --osc-script option)" 1>&2
    exit 1
fi

session="all.sv"

if [ ! -f "$session" ]; then
    echo "Session file $session not found" 1>&2
    exit 1
fi

tmpdir=$(mktemp -d)
trap "rm -rf $tmpdir" 0

input="$tmpdir/input.sv"

cp "$session" "$input"

cat > "$tmpdir/script" <<EOF
/open "$input"
/setcurrent 1 3
/exportlayer "$tmpdir/instants.csv"
/setcurrent 2 3
/exportlayer "$tmpdir/values.csv"
/setcurrent 3 2
/exportlayer "$tmpdir/image.csv"
/setcurrent 3 3
/exportlayer "$tmpdir/regions.csv"
/setcurrent 4 2
/exportlayer "$tmpdir/text.csv"
/setcurrent 4 3
/exportlayer "$tmpdir/notes.csv"
/setcurrent 5 2
/exportlayer "$tmpdir/3dplot.csv"
/setcurrent 6 2
/exportlayer "$tmpdir/spectrogram.csv"
/setcurrent 6 3
/exportlayer "$tmpdir/boxes.csv"
/setcurrent 7 2
/exportlayer "$tmpdir/peakfreq.csv"
/quit
EOF

"$sv" --no-splash --osc-script "$tmpdir/script"

for type in instants values image regions text notes 3dplot spectrogram boxes peakfreq ; do
    actual="$tmpdir/$type.csv"
    expected="layers-expected/$type.csv"
    if ! cmp -s "$actual" "$expected" ; then
        echo
        echo "Test failed for layer type \"$type\"!"
        echo
        echo "Actual:"
        ls -l "$actual"
        echo "Expected"
        ls -l "$expected"
        echo
        echo "Diff begins:"
        diff -u1 "$actual" "$expected" | head 
        echo
    fi
done


