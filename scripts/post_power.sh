#!/bin/bash

#@file:   scripts/post_power.sh 
#@brief:  Post-processing output of powerwrapper 
#@author: Ippokratis Pandis

# args: <file>
if [ $# -lt 1 ]; then
    echo "Usage: $0 <file>" >&2
    exit 1
fi

EXPFILE=$1; shift

echo "MQTh"
cat $EXPFILE | ggrep -e "MQTh" -e "measure" | grep -v "measurement" | sed 's/^.*(//' | sed -e 's/.$//'
echo "SuccessRate"
cat $EXPFILE | grep "Success" | uniq | sed 's/^.*(//' | sed -e 's/..$//'

exit

