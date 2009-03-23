#!/bin/bash

#@file:   scripts/post_power_tpcc.sh 
#@brief:  Post-processing output of powerwrapper for TPC-C
#@author: Ippokratis Pandis

# args: <file>
if [ $# -lt 1 ]; then
    echo "Usage: $0 <file>" >&2
    exit 1
fi

EXPFILE=$1; shift

echo "TPS"
cat $EXPFILE | ggrep -e "TPS" -e "measure" | grep -v "measurement" | sed 's/^.*(//' | sed -e 's/.$//'

exit

