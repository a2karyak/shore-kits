#!/bin/bash

#@file:   scripts/powerwrapper.sh 
#@brief:  Runs a single experiment, varying the number of clients. The user
#         specifies the space of the client values from a set of predefined values
#@author: Ippokratis Pandis

# typical usage: 
#
# DORA-TM1
# for ((trx=220; trx <= 226; trx++)); do ./scripts/powerwrapper.sh EXP basetm1 100 1 80 $trx 30 3; done

# args: <base-dir> <name> <low> <high> <xctid> <time> <iter>
if [ $# -lt 7 ]; then
    echo "Usage: $0 <base-dir> <workload-name> <low-cl> <high-cl> <xctid> <time> <iter>" >&2
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

EXPSHELL="scripts/trx_powerrun.sh"
TRXSHELL="tests/shore_kits"

mkdir -p $BASE_DIR

CMD="source $EXPSHELL $LOW $HIGH $XCT $TIME $ITER | $TRXSHELL 2>&1 | tee $OUTFILE"
echo "$CMD" | tee $OUTFILE
($EXPSHELL $LOW $HIGH $XCT $TIME $ITER | $TRXSHELL) 2>&1 | tee -a $OUTFILE
