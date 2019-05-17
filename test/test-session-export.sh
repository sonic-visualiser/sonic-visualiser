#!/bin/bash
#
# Test that loading and re-saving a session does not change its contents
# Must be run from same directory as the SV binary

set -e

session="$1"

set -u

sv="../sonic-visualiser"
if [ ! -x "$sv" ]; then
    echo "This script must be run from the sonic-visualiser/test directory" 1>&2
    exit 1
fi

if ! xmllint --version 2>/dev/null ; then
    echo "Can't find required xmllint program (from libxml2 distribution)" 1>&2
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

if [ -z "$session" ]; then
    echo "Usage: $0 <session.sv>" 1>&2
    exit 2
fi

if [ ! -f "$session" ]; then
    echo "Session file $session not found" 1>&2
    exit 1
fi

tmpdir=$(mktemp -d)
trap "rm -rf $tmpdir" 0

input="$tmpdir/input.sv"
inxml="$tmpdir/input.xml"
output="$tmpdir/output.sv"
outxml="$tmpdir/output.xml"

cp "$session" "$input"

cat > "$tmpdir/script" <<EOF
/open "$input"
/save "$output"
/quit
EOF

"$sv" --no-splash --osc-script "$tmpdir/script"

if [ ! -f "$output" ]; then
    echo "ERROR: Failed to save session to $output at all!" 1>&2
    exit 1
fi

bunzip2 -c "$input" | xmllint --format - > "$inxml"
bunzip2 -c "$output" | xmllint --format - > "$outxml"

diff -u "$inxml" "$outxml"

