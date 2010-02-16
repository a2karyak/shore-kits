#!/bin/bash

#@file:   scripts/post_logger.sh 
#@brief:  Post-processing output of powerwrapper for the logger-related runs
#@author: Ippokratis Pandis

# args: <file>
if [ $# -lt 1 ]; then
    echo "Usage: $0 <file>" >&2
    exit 1
fi

EXPFILE=$1; shift

### TPCC/TPCB have TPS, while TM1 has MQTh
echo "Throughput"
cat $EXPFILE | ggrep -e "TPS" -e "MQTh" -e "measure" | grep -v "measurement" | sed 's/^.*(//' | sed -e 's/.$//'

echo "AvgCPU"
cat $EXPFILE | ggrep -e "AvgCPU" -e "measure" | grep -v "measurement" | sed 's/^.*(//' | sed -e 's/..$//'

echo "CpuLoad"
cat $EXPFILE | ggrep -e "CpuLoad" -e "measure" | grep -v "measurement" | sed 's/^.*(//' | sed -e 's/.$//'

echo "Voluntary Ctx"
cat $EXPFILE | ggrep -e "Voluntary" -e "measure" | grep -v "measurement" | sed 's/^.*\. //'

echo "Involuntary Ctx"
cat $EXPFILE | ggrep -e "Involuntary" -e "measure" | grep -v "measurement" | sed 's/^.*\. //'

### TM1 has also SuccessRate
echo "SuccessRate"
cat $EXPFILE | grep "Success" | uniq | sed 's/^.*(//' | sed -e 's/..$//'


exit

