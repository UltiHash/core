#!/bin/bash

#
# Help text
#
usage () {
    echo "usage: mk_include_file.sh <key> <include>"
    echo
    echo "<key>  - path to public key file to include in application"
    echo "<include> - path to include file to generate"
}

#
# Command line parsing
#
key="$1"
include="$2"

[ -n "$include" ] || { usage; exit 1; }
[ -f "$key" ] || { echo "cannot open $key"; exit 1; }

#
# Perform action
#
xxd -i -n UH_LICENSE_PUBLIC_KEY $key > $include
