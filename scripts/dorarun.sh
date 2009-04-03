#!/bin/bash

#@file:   dorarun.sh 
#
#@brief:  Does a dbx run for DORA
#@note:   Assumes that the db is already populated
#
#@author: Ryan Johnson
#@author: Ippokratis Pandis

# Modified for DORA. It does not spread threads (SF==CLIENTS)
# Removed the SLI option

# args: <dir> <name> <xctid> <sf> <length> <iterations> <warmup_mix>

if [ $# -lt 7 ]; then
    echo "Invalid args: $0 $*" >&2
    echo "Usage: $0 <dir> <name> <xctid> <sf> <measure-time> <measure-iters> <warmup-trx>" >&2
    echo " " >&2
    echo "Wrapper script does not pass: <sf> <measure-time> <measure-iters> <warmup-trx>" >&2
    exit 1
fi


DIR=$1; shift
NAME=$1; shift
XCT=$1; shift
SF=$1; shift
TIME=$1; shift
ITER=$1; shift
WARMUP_MIX=$1; shift

command()
{
    echo echo $@
    echo $@
}

### dbx
START_EXP=startup.100.er
command collector archive copy
command collector store directory $DIR
command collector store experiment $START_EXP
command collector enable
command stop in main
command run
command collector disable
command collector archive off # we'll manually link in later...

### kit

### dbx
#command assign signal_debugger_when_done=1
#command assign show_sli_lock_info=0
command cont

# this sleep needs to be long enough to load the db!
# (ip) No need for db loading
sleep 60

### kit
command measure $SF 0 10 30 $WARMUP_MIX 3
command break
command collector disable

#sleep 30


run_one ()
{
    # <clients>>
    CLIENTS=$1
    
    EXPNAME=$NAME-$XCT-$(printf "%02d" $CLIENTS)cl.100.er
### dbx
    command collector store experiment $EXPNAME
    command collector enable
    command cont
    # make sure to get all the measurements before continuing!
    sleep $((20+TIME*ITER))
    
    ### kit
    command measure $CLIENTS 1 $CLIENTS $TIME $XCT $ITER
    command break
    command collector disable
    # link in the archive from the base experiment
    (cd $DIR/$EXPNAME; rm -r archives; ln -s ../$START_EXP/archives)
}

# tm1-sli sequence
#CLIENT_SEQ=(1 3 7 15 23 31 35 39 43 47 51 55 59 63)

# test
#CLIENT_SEQ=(1 3)

# tm1-dora sequence
#CLIENT_SEQ=(1 2 4 8 16 24 32 40 46 52 58 64 68 74 78)
#CLIENT_SEQ=(1 2 4 8 16 24 32 40 46 52 58 64)
CLIENT_SEQ=(1 2 4 8 16 24 32 40 46 52)

#CLIENT_SEQ=(58 64)

#!! dora-payment seq
#CLIENT_SEQ=(10 20 40 60 80 100 120)

for i in ${CLIENT_SEQ[@]}; do
    run_one $i
done

### dbx
command cont
sleep 1

### kit
command quit


### dbx
command kill
command exit
exit
    