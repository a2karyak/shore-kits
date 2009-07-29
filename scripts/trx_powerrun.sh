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

# instead of warmup we set clobberdev = 1 (load a new database each time) 
# and sleep for the:
# TM1 : 5mins   (100sf)
#sleep 300
# TPCC: 20mins  (100wh)
 sleep 1200
# TPCC: 12mins  (50/60wh)
#sleep 720
# TPCB: 4mins   (100wh)
# sleep 250
# TPCB: 2mins   (60wh)
# sleep 120


run_one ()
{
    # <clients>
    CLIENTS=$1
    
    ### kit
    command measure $CLIENTS 1 $CLIENTS $TIME $XCT $ITER

    # make sure to get all the measurements before continuing!
    sleep $((5+TIME*ITER))
}


###########################################################
#
# BASELINE 
#
###########################################################

# TM1-SLI
#CLIENT_SEQ=(1 3 7 15 23 31 35 39 43 47 51 55 59 63)

# TPCC/TPCB/TM1 - BASE
#CLIENT_SEQ=(1 4 8 16 24 32 36 40 44 48 52 56 58 64 68 74 78)
#CLIENT_SEQ=(68 72 76 80 84)



###########################################################
#
# DORA 
#
###########################################################

# TPC-C - DORA
CLIENT_SEQ=(1 4 8 12 16 20 24 28 32 36 40 44 48 50 54 58)
#CLIENT_SEQ=(28 32 36 40 44 48 50)

# TPC-B - DORA
#CLIENT_SEQ=(1 4 8 12 16 20 24 28 32 36 40 44 48)

# TM1-RO - DORA
#CLIENT_SEQ=(1 4 8 16 24 32 36 40 44 48 52 56 58 64 68)

# TM1-RW (UpdSubData,UpdLocation) - DORA
#CLIENT_SEQ=(1 4 8 16 24 32 36 40 44 48 52 56 58 64 68 72 76 78 82 84)

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

