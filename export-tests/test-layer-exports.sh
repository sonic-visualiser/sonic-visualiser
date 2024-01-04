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

sv="../build/sonic-visualiser"
if [ ! -f "$sv" -o ! -x "$sv" ]; then
    echo "This script must be run from the sonic-visualiser/export-tests directory" 1>&2
    echo "It expects a sonic-visualiser binary to be found in ./build/" 1>&2
    exit 1
fi

version=$("$sv" -v 2>&1 | grep -v App)
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
# Load the session file
/open "$input"

# Select each exportable layer in turn and export to a CSV file
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

# Note layer can also be exported as MIDI
/setcurrent 4 3
/exportlayer "$tmpdir/notes.mid"

# And everything as SVL
/setcurrent 1 3
/exportlayer "$tmpdir/instants.svl"
/setcurrent 2 3
/exportlayer "$tmpdir/values.svl"
/setcurrent 3 2
/exportlayer "$tmpdir/image.svl"
/setcurrent 3 3
/exportlayer "$tmpdir/regions.svl"
/setcurrent 4 2
/exportlayer "$tmpdir/text.svl"
/setcurrent 4 3
/exportlayer "$tmpdir/notes.svl"
/setcurrent 5 2
/exportlayer "$tmpdir/3dplot.svl"
/setcurrent 6 2
/exportlayer "$tmpdir/spectrogram.svl"
/setcurrent 6 3
/exportlayer "$tmpdir/boxes.svl"
/setcurrent 7 2
/exportlayer "$tmpdir/peakfreq.svl"

# Now test exporting only the contents of a (multiple) selection. This
# is only supported for CSV files.
# First set waveform layer as current, to avoid snapping the selection
# to the contents of an annotation layer.
/setcurrent 1 2

# Make a selection
/select 8 10
/addselect 14 16

# And repeat all the previous exports
/setcurrent 1 3
/exportlayer "$tmpdir/selected-instants.csv"
/setcurrent 2 3
/exportlayer "$tmpdir/selected-values.csv"
/setcurrent 3 2
/exportlayer "$tmpdir/selected-image.csv"
/setcurrent 3 3
/exportlayer "$tmpdir/selected-regions.csv"
/setcurrent 4 2
/exportlayer "$tmpdir/selected-text.csv"
/setcurrent 4 3
/exportlayer "$tmpdir/selected-notes.csv"
/setcurrent 5 2
/exportlayer "$tmpdir/selected-3dplot.csv"
/setcurrent 6 2
/exportlayer "$tmpdir/selected-spectrogram.csv"
/setcurrent 6 3
/exportlayer "$tmpdir/selected-boxes.csv"
/setcurrent 7 2
/exportlayer "$tmpdir/selected-peakfreq.csv"

/setcurrent 4 3
/exportlayer "$tmpdir/selected-notes.mid"

# If we also zoom in vertically in the 3d plot, our export should
# include only the zoomed area - check this
/setcurrent 5 2
/zoomvertical 0 12
/exportlayer "$tmpdir/selected-zoomed-3dplot.csv"

/quit
EOF

"$sv" --no-splash --osc-script "$tmpdir/script"

for type in instants values image regions text notes 3dplot spectrogram boxes peakfreq ; do
    for format in csv svl ; do
        for pfx in "" "selected-"; do
            if [ "$format" = "svl" ] && [ -n "$pfx" ]; then
                continue
            fi
            actual="$tmpdir/$pfx$type.$format"
            expected="layers-expected/$pfx$type.$format"
            if ! cmp -s "$actual" "$expected" ; then
                echo
                if [ -z "$pfx" ]; then
                    echo "Test failed for file type $format, layer type \"$type\"!"
                else
                    echo "Test failed for selected regions in layer type \"$type\"!"
                fi
                echo
                echo "Actual:"
                ls -l "$actual"
                echo "Expected:"
                ls -l "$expected"
                echo
                echo "Diff begins:"
                git diff --no-index --word-diff=color --word-diff-regex=. "$actual" "$expected" | head
                echo
            fi
        done
    done
done

for csv in selected-zoomed-3dplot.csv ; do
    actual="$tmpdir/$csv"
    expected="layers-expected/$csv"
    if ! cmp -s "$actual" "$expected" ; then
        echo
        echo "Test failed for \"$csv\"!"
        echo
        echo "Actual:"
        ls -l "$actual"
        echo "Expected:"
        ls -l "$expected"
        echo
        echo "Diff begins:"
        git diff --no-index --word-diff=color --word-diff-regex=. "$actual" "$expected" | head
        echo
    fi
done

for other in notes.mid selected-notes.mid ; do
    actual="$tmpdir/$other"
    expected="layers-expected/$other"
    if ! cmp -s "$actual" "$expected" ; then
        echo
        if [ -z "$pfx" ]; then
            echo "Test failed for \"$other\"!"
        fi            
        echo
        echo "Actual:"
        ls -l "$actual"
        echo "Expected:"
        ls -l "$expected"
        echo
        od -c "$actual" > "$actual".txt
        od -c "$expected" > "$tmpdir/expected-$other".txt
        echo
        echo "Diff:"
        git diff --no-index --word-diff=color --word-diff-regex=. "$actual".txt "expected-$other".txt | head
        echo
    fi
done

