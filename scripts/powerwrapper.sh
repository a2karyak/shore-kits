#!/bin/bash

#@file:   scripts/powerwrapper.sh 
#@brief:  Runs a single experiment, varying the number of clients. The user
#         specifies the space of the client values from a set of predefined values
#@author: Ippokratis Pandis

# typical usage: 
#
# DORA-TM1
# for ((trx=220; trx <= 226; trx++)); do ./scripts/powerwrapper.sh EXP tm1-100-nl 1 80 $trx 30 3; done
# or
# SEQ=(224 226 200) ; for i in ${SEQ[@]}; do ./scripts/powerwrapper.sh EXP tm1-100-nl 1 80 $i 30 3; done


EXPSHELL="scripts/trx_powerrun.sh"
TRXSHELL="shore_kits"
SHORECONF="shore.conf"

# args: <base-dir> <selecteddb> <low> <high> <xctid> <time> <iter> <sleeptime>
if [ $# -lt 8 ]; then
    echo "Usage: $0 <base-dir> <selecteddb> <low-cl> <high-cl> <xctid> <time> <iter> <sleeptime> <clientset>" >&2
    echo " " >&2
    echo "Examples:" >&2
    echo "BASE-TM1: for ((trx=20; trx <= 26; trx++)); do ./scripts/powerwrapper.sh EXP tm1-100-nl 1 80 \$trx 30 3 300 large ; done" >&2
    echo "DORA-TM1: for ((trx=220; trx <= 226; trx++)); do ./scripts/powerwrapper.sh EXP tm1-100 1 80 \$trx 30 3 300 large ; done" >&2
    echo "SEQ=(224 226 200) ; for trx in \${SEQ[@]}; do ./scripts/powerwrapper.sh EXP tm1-100 1 80 \$trx 30 3 300 large ; done" >&2
    echo " " >&2
    exit 1
fi

BASE_DIR=$1; shift
SELECTEDDB=$1; shift
LOW=$1; shift
HIGH=$1; shift
XCT=$1; shift
TIME=$1; shift
ITER=$1; shift
SLEEPTIME=$1; shift
CLIENTSET=$1; shift

STAMP=$(date +"%F-%Hh%Mm%Ss")
OUTFILE=$BASE_DIR/perf-$SELECTEDDB-$XCT.$STAMP.out

mkdir -p $BASE_DIR

# remove log/unilog
echo rm -rf log/*
rm -rf log/*
echo rm -rf unilog/*
rm -rf unilog/*

# delete all databases !
echo rm -rf databases/*
rm -rf databases/*


CMD="source $EXPSHELL $LOW $HIGH $XCT $TIME $ITER | $TRXSHELL $SELECTEDDB 2>&1 | tee $OUTFILE"
echo "$CMD" | tee $OUTFILE
echo "Configuration" | tee -a $OUTFILE
cat shore.conf | grep "^$SELECTEDDB" | tee -a $OUTFILE 
($EXPSHELL $LOW $HIGH $XCT $TIME $ITER $SLEEPTIME $CLIENTSET | $TRXSHELL $SELECTEDDB) 2>&1 | tee -a $OUTFILE

