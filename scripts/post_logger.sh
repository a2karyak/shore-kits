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

OUTFILE=filter.$EXPFILE
echo $OUTFILE

### TPCC/TPCB have TPS, while TM1 has MQTh
echo "++ Throughput" | tee $OUTFILE
cat $EXPFILE | ggrep -e "TPS" -e "MQTh" -e "measure" | grep -v "measurement" | sed 's/^.*(//' | sed -e 's/.$//' | tee -a $OUTFILE

echo "++ AvgCPU" | tee -a $OUTFILE
cat $EXPFILE | ggrep -e "AvgCPU" -e "measure" | grep -v "measurement" | sed 's/^.*(//' | sed -e 's/..$//' | tee -a $OUTFILE

echo "++ CpuLoad" | tee -a $OUTFILE
cat $EXPFILE | ggrep -e "CpuLoad" -e "measure" | grep -v "measurement" | sed 's/^.*(//' | sed -e 's/.$//' | tee -a $OUTFILE

echo "++ Voluntary Ctx" | tee -a $OUTFILE
cat $EXPFILE | ggrep -e "Voluntary" -e "measure" | grep -v "measurement" | sed 's/^.*\. //' | tee -a $OUTFILE

echo "++ Involuntary Ctx" | tee -a $OUTFILE
cat $EXPFILE | ggrep -e "Involuntary" -e "measure" | grep -v "measurement" | sed 's/^.*\. //' | tee -a $OUTFILE

echo "++ Total User" | tee -a $OUTFILE
cat $EXPFILE | ggrep -e "Total User" -e "measure" | grep -v "measurement" | sed 's/^.*\. //' | tee -a $OUTFILE

echo "++ Total System" | tee -a $OUTFILE
cat $EXPFILE | ggrep -e "Total System" -e "measure" | grep -v "measurement" | sed 's/^.*\. //' | tee -a $OUTFILE

echo "++ Other System" | tee -a $OUTFILE
cat $EXPFILE | ggrep -e "Other System" -e "measure" | grep -v "measurement" | sed 's/^.*\. //' | tee -a $OUTFILE

#rm $OUTFILE
exit

