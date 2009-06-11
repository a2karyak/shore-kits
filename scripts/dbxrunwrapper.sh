#!/bin/bash

#@file:   dbxrunwrapper.sh 
#@brief:  Wrapper for the dbx runs. Creates/opens directories/files, pipes commands.
#@author: Ryan Johnson

# typical usage: 
#
# SLI-TM1
# for ((trx=20; trx <= 26; trx++)); do ./scripts/dbxrunwrapper.sh sli tm1 $trx ./scripts/slirun.sh 10000 10 3 20; done
#
# DORA-TM1
# for ((trx=220; trx <= 226; trx++)); do ./scripts/dbxrunwrapper.sh exp tm1dora $trx ./scripts/dorarun.sh 100 30 3 200; done
# !! Note for DORA-TM1
# Cannot distinguish between DORA and BASELINE. Therefore, 
# (1) Edit shore.conf to point to the correct configuration (DORA or BASELINE)
# (2) Fire up test with different workload-name (doratm1, basetm1)


# args: <output-dir> <name> <xctid> <run-script> [<run-script-args>...]
if [ $# -lt 4 ]; then
    echo "Usage: $0 <base-dir> <workload-name> <xctid> <run-script> [args for run-script...]" >&2
    echo -e "\t the run-script will be passed the output-dir, workload-name, the xctid, and any remaining args" >&2
    exit 1
fi

BASE_DIR=$1; shift
NAME=$1; shift
XCT=$1; shift
SCRIPT=$1; shift

STAMP=$(date +"%F-%Hh%Mm%Ss")
DIR=$BASE_DIR/run-$NAME-$XCT.$STAMP
OUTFILE=$DIR/run.out

TRXSHELL="shore_kits"
#TRXSHELL="tests/tpcc_kit_shell"

#DBX="/opt/SSX0903/bin/dbx"
DBX="dbx"

mkdir -p $DIR

# remove log 
echo rm log/*
rm log/*

CMD="source $SCRIPT $DIR $NAME $XCT $* | $DBX $TRXSHELL 2>&1 | tee $OUTFILE"
echo "$CMD" | tee $OUTFILE
($SCRIPT $DIR $NAME $XCT $* | $DBX $TRXSHELL) 2>&1 | tee -a $OUTFILE
