#!/bin/bash

target=$1

if [ ! -f "$target" ]; then 
    echo "Usage: $0 target-executable"
    exit 1
fi

pfile=/tmp/packages_$$
rfile=/tmp/redundant_$$

trap "rm -f $pfile $rfile" 0
echo 1>&2

ldd "$target" | awk '{ print $3; }' | grep '^/' | while read lib; do
    if test -n "$lib" ; then
	dpkg-query -S "$lib"
    fi
    done | grep ': ' | awk -F: '{ print $1 }' | sort | uniq > $pfile

echo "Packages providing required libraries:" 1>&2
cat $pfile 1>&2
echo 1>&2

for p in `cat $pfile`; do 
    echo Looking at $p 1>&2
    apt-cache showpkg "$p" | grep '^  ' | grep ',' | awk -F, '{ print $1; }' | \
	while read d; do 
	    if grep -q '^'$d'$' $pfile; then
		echo $p
	    fi
    done
done | sort | uniq > $rfile

echo "Packages that can be eliminated because other packages depend on them:" 1>&2
cat $rfile 1>&2
echo 1>&2

cat $pfile $rfile | sort | uniq -u | sed 's/$/,/' | fmt -1000 | sed 's/^/Depends: /' | sed 's/,$/, libc6/' | sed 's/libjack0,/jackd,/'


