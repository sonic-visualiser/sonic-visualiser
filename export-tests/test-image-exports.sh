#!/bin/bash
#
# Regression tests for image and SVG export
# Must be run from directory that contains this script

# Based on test-layer-exports.sh, see that script for further comments

set -e

sv="../build/sonic-visualiser"

if [ -n "$1" ]; then
    sv="$1"
    shift
fi

usage() {
    echo 1>&2
    echo "Usage: $0 [/optional/path/to/sonic-visualiser]" 1>&2
}

if [ -n "$1" ]; then
    usage
    exit 2
fi

set -u

if [ ! -d "../export-tests" ]; then
    usage
    echo 1>&2
    echo "This script must be run from the sonic-visualiser/export-tests directory" 1>&2
    exit 1
fi

if [ ! -f "$sv" -o ! -x "$sv" ]; then
    usage
    echo 1>&2
    echo "Could not find sonic-visualiser." 1>&2
    echo "If no sonic-visualiser binary is specified on the command line, we expect to" 1>&2
    echo "find one in ../build/sonic-visualiser ." 1>&2
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

outdir="image-output"

mkdir -p "$outdir"
rm -f "$outdir"/*.png "$outdir"/*.svg

input="$outdir/input.sv"

cp "$session" "$input"

cat > "$outdir/script" <<EOF
# Load the session file
/open "$input"

# Select each pane in turn and export to an image
/setcurrent 1 3
/exportimage "$outdir/pane1.png"
/setcurrent 2 3
/exportimage "$outdir/pane2.png"
/setcurrent 3 3
/exportimage "$outdir/pane3.png"
/setcurrent 4 3
/exportimage "$outdir/pane4.png"
/setcurrent 5 2
/exportimage "$outdir/pane5.png"
/setcurrent 6 3
/exportimage "$outdir/pane6.png"
/setcurrent 7 2
/exportimage "$outdir/pane7.png"

# And as SVG
/setcurrent 1 3
/exportsvg "$outdir/pane1.svg"
/setcurrent 2 3
/exportsvg "$outdir/pane2.svg"
/setcurrent 3 3
/exportsvg "$outdir/pane3.svg"
/setcurrent 4 3
/exportsvg "$outdir/pane4.svg"
/setcurrent 5 2
/exportsvg "$outdir/pane5.svg"
/setcurrent 6 3
/exportsvg "$outdir/pane6.svg"
/setcurrent 7 2
/exportsvg "$outdir/pane7.svg"

# Make a selection
/select 8 10
/addselect 14 16

# And repeat all the previous exports

/setcurrent 1 3
/exportimage "$outdir/pane1_selection.png"
/setcurrent 2 3
/exportimage "$outdir/pane2_selection.png"
/setcurrent 3 3
/exportimage "$outdir/pane3_selection.png"
/setcurrent 4 3
/exportimage "$outdir/pane4_selection.png"
/setcurrent 5 2
/exportimage "$outdir/pane5_selection.png"
/setcurrent 6 3
/exportimage "$outdir/pane6_selection.png"
/setcurrent 7 2
/exportimage "$outdir/pane7_selection.png"

/setcurrent 1 3
/exportsvg "$outdir/pane1_selection.svg"
/setcurrent 2 3
/exportsvg "$outdir/pane2_selection.svg"
/setcurrent 3 3
/exportsvg "$outdir/pane3_selection.svg"
/setcurrent 4 3
/exportsvg "$outdir/pane4_selection.svg"
/setcurrent 5 2
/exportsvg "$outdir/pane5_selection.svg"
/setcurrent 6 3
/exportsvg "$outdir/pane6_selection.svg"
/setcurrent 7 2
/exportsvg "$outdir/pane7_selection.svg"


# If we also zoom in vertically in the 3d plot, our export should
# include only the zoomed area - check this
/setcurrent 5 2
/zoomvertical 0 12
/exportimage "$outdir/pane5_selected_zoomed.png"
/exportsvg "$outdir/pane5_selected_zoomed.svg"

/quit
EOF

"$sv" --no-splash --osc-script "$outdir/script"

xdg-open "$outdir"
