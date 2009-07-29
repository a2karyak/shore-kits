#!/bin/bash

#@file:   scripts/powerwrapper.sh 
#@brief:  Runs a single experiment, varying the number of clients. The user
#         specifies the space of the client values from a set of predefined values
#@author: Ippokratis Pandis

# typical usage: 
#
# DORA-TM1
# for ((trx=220; trx <= 226; trx++)); do ./scripts/powerwrapper.sh EXP dora-tm1 1 80 $trx 30 3; done
# or
# SEQ=(224 226 200) ; for i in ${SEQ[@]}; do ./scripts/powerwrapper.sh EXP dora-tm1 1 80 $i 30 3; done


EXPSHELL="scripts/trx_powerrun.sh"
TRXSHELL="shore_kits"
SHORECONF="shore.conf"

# args: <base-dir> <name> <low> <high> <xctid> <time> <iter>
if [ $# -lt 7 ]; then
    echo "Usage: $0 <base-dir> <workload-name> <low-cl> <high-cl> <xctid> <time> <iter>" >&2
    echo " " >&2
    echo "Examples:" >&2
    echo "BASE-TM1: for ((trx=20; trx <= 26; trx++)); do ./scripts/powerwrapper.sh EXP base-tm1 1 80 \$trx 30 3; done" >&2
    echo "DORA-TM1: for ((trx=220; trx <= 226; trx++)); do ./scripts/powerwrapper.sh EXP dora-tm1 1 80 \$trx 30 3; done" >&2
    echo "SEQ=(224 226 200) ; for trx in \${SEQ[@]}; do ./scripts/powerwrapper.sh EXP dora-tm1 1 80 \$trx 30 3; done" >&2
    echo " " >&2
    echo "!!! Make sure that shore.conf has the correct configuration for your experiment !!!" >&2
    echo "Current configuration:" >&2
    cat $SHORECONF | grep "^db-config" >&2
    echo " " >&2
    echo "!!! Make sure that $EXPSHELL will sleep enough for the database to populate !!!" >&2
    echo "Current sleep:" >&2
    cat $EXPSHELL | grep "^sleep" >&2
    echo " " >&2
    echo "!!! Make sure that $EXPSHELL has the right sequence of clients for your experiment !!!" >&2
    echo "Current sequence:" >&2
    cat $EXPSHELL | grep "^CLIENT_SEQ=" >&2
    echo " " >&2
    exit 1
fi

BASE_DIR=$1; shift
NAME=$1; shift
LOW=$1; shift
HIGH=$1; shift
XCT=$1; shift
TIME=$1; shift
ITER=$1; shift


STAMP=$(date +"%F-%Hh%Mm%Ss")
OUTFILE=$BASE_DIR/perf-$NAME-$XCT.$STAMP.out

mkdir -p $BASE_DIR

# remove log/unilog
echo rm -rf log/*
rm -rf log/*
echo rm -rf unilog/*
rm -rf unilog/*



CMD="source $EXPSHELL $LOW $HIGH $XCT $TIME $ITER | $TRXSHELL 2>&1 | tee $OUTFILE"
echo "$CMD" | tee $OUTFILE
($EXPSHELL $LOW $HIGH $XCT $TIME $ITER | $TRXSHELL) 2>&1 | tee -a $OUTFILE

