#!/bin/bash

#@file:   scripts/trx_powerrun.sh 
#@brief:  For a specific xct it executes powerruns for a set of different clients
#@author: Ippokratis Pandis

# args: <low> <high> <xctid> <time> <iter>
usage()
{
    echo "Usage: $0 <low> <high> <xctid> <time> <iter>" >&2
}

if [ $# -lt 5 ]; then
    echo "Invalid args: $0 $*" >&2
    usage
    exit 1
fi

LOW=$1; shift
HIGH=$1; shift
XCT=$1; shift
TIME=$1; shift
ITER=$1; shift


if [ $HIGH -lt $LOW ]; then
    echo "Invalid Input: $LOW $HIGH" >&2    
    usage
    exit 1
fi


command()
{
    echo echo $@
    echo $@
}

### kit



#warmup
command measure $HIGH 0 20 10 $XCT 2
sleep 20

# TPC-C Warmup
# sleep 60
# command measure $HIGH 0 36 60 $XCT 3
# sleep 200


run_one ()
{
    # <clients>
    CLIENTS=$1
    
    ### kit
    command measure $CLIENTS 1 $CLIENTS $TIME $XCT $ITER

    # make sure to get all the measurements before continuing!
    sleep $((5+TIME*ITER))
}

# tm1-sli sequence
#CLIENT_SEQ=(1 3 7 15 23 31 35 39 43 47 51 55 59 63)

# test
#CLIENT_SEQ=(1 3)

# tm1-dora sequence
#CLIENT_SEQ=(1 2 4 8 16 24 32 40 46 52 58 64 68 74 78 84 92 96 100)
CLIENT_SEQ=(1 2 4 8 16 24 32 40 46 52 58 64)

for i in ${CLIENT_SEQ[@]}; do
    if [ $i -ge $LOW ]; then
        if [ $i -le $HIGH ]; then
            run_one $i
        fi
    fi
done

### kit
command quit
exit
    