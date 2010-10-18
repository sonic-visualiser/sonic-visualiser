#!/bin/bash

target=$1

if [ ! -f "$target" ]; then 
    echo "Usage: $0 target-executable"
    exit 1
fi

pfile=/tmp/packages_$$
rfile=/tmp/redundant_$$

trap "rm -f $pfile $rfile" 0
echo

ldd "$target" | awk '{ print $3; }' | while read lib; do
    if test -n "$lib" ; then
	dpkg-query -S "$lib"
    fi
    done | grep ': ' | awk -F: '{ print $1 }' | sort | uniq > $pfile

echo "Packages providing required libraries:"
cat $pfile
echo

for p in `cat $pfile`; do 
    apt-cache showpkg "$p" | grep '^  ' | grep ',' | awk -F, '{ print $1; }' | \
	while read d; do 
	    if grep -q '^'$d'$' $pfile; then
		echo $p
	    fi
    done
done | sort | uniq > $rfile

echo "Packages that can be eliminated because other packages depend on them:"
cat $rfile
echo

echo "Remaining required packages:"
cat $pfile $rfile | sort | uniq -u

