#!/bin/bash

# Requirements:
# - qpipe must be running and sending output to the file qpipe.out
# - qpipe must also be reading input from qpipe.in
# - dbx must be running and reading input from the pipe dbx.in

# Functionality:
# Creates an instance of qpipe that reads from a pipe and writes to a file
# Creates an instance of dbx and attaches it to qpipe
# Accepts commands from stdin and sends them to qpipe one at a time
# For each command:
# - set up a new profile run
# - send the command to qpipe
# - wait for the command to complete
# - stop profiling
# - writes the command and its throughput to a file

DIR=.
function errcho()
{
    echo "$@" >> $DIR/script.out
    echo "$@" >&2
}


# sets PID to the desired process, if found, or NULL
function find_pid()
{
    # only allow exact matches at end of line
    PID=$(ps -u$USER | grep " $1$")
    RESULT=$?
#    errcho "PID search returned $PID with code $RESULT"
    if [ $RESULT -ne 0 ]; then
	errcho "Unable to find a running instance of $1"
	exit 1
    fi
    if [ $(echo "$PID" | wc -l) -gt 1 ]; then
	errcho "Found multiple instances of $1. Chickening out"
	exit 1
    fi
    PID=$(echo $PID | awk '{print $1}')
}

# wait for the qpipe process to start
sleep 1


# extract parameters from command-line
if [ $# -ne 3 ]; then
    cat >&2 <<EOF
Usage:
\$ collect-stats.sh <query> <cores> <dest-dir> < qpipe.in | ./qpipe > qpipe.out

This script *must* be piped to the input of qpipe to control it.
The script saves all input and output streams as log files in the dest-dir.
qpipe *must* write its output to 'qpipe.out' for this script to read.
EOF
    find_pid qpipe
    errcho "Killing qpipe ($PID)"
    kill $PID
    exit 1
fi

QUERY=$1
N=$2
DIR=$3

#globals
MLIST="1 2 4 7 11 16 22 29 37 45" 
REPEAT=3

# overrides for testing
#MLIST="1 2 3"
#REPEAT=2

mkdir -p $DIR
# serialize this run
SERFILE=$DIR/serial.txt
if [ ! -e $SERFILE ]; then
    echo 0 > $SERFILE
fi

read SERIAL < $SERFILE
((SERIAL++))
echo $SERIAL > $SERFILE
SERIAL=$(printf "ser%04d" $SERIAL)

# create a serialized dir to store output, then soft-link it for ease of use
SUBDIR=$(printf "%s-n%02d" $QUERY $N)
SERDIR=$SUBDIR-$SERIAL
(
    mkdir -p $DIR
    cd $DIR
    mkdir $SERDIR
    rm -f $SUBDIR
    ln -s $SERDIR $SUBDIR
)
DIR=$DIR/$SUBDIR
SERDIR=$DIR/$SERDIR

find_pid qpipe
QPIPE_PID=$PID
errcho "Found qpipe with PID $QPIPE_PID"

FAIL_PSRSET=no
if [ "$N" -lt 32 ]; then
    errcho "Giving the user a chance to bind qpipe to a processor set of size $N..."
    sleep 10
    errcho "... checking processor set"
    PSRSET=$(/usr/sbin/psrset -q $QPIPE_PID)
    if [ -n "$PSRSET" ]; then
	errcho "qpipe is not bound to a processor set and N=$N"
	FAIL_PSRSET=yes
    else
	PSRSET_SIZE=$(/usr/sbin/psrset -i $PSRSET | wc -w)
	# The output of psrset is "user processor set N: ..."
	((PSRSET_SIZE -= 4))
	if [ "$PSRSET_SIZE" -ne "$N" ]; then
	    errcho "qpipe bound to a processor set of size $PSRSET_SIZE when N=$N"
	    FAIL_PSRSET=yes
	fi
    fi
else
    # make usre there are no processor sets defined
    if [ ! -n "$(/usr/sbin/psrset)" ]; then
	errcho "processor sets exist when N=32"
	FAIL_PSRSET=yes
    fi
fi

if [ "$FAIL_PSRSET" == "yes" ]; then
    errcho "killing qpipe"
    kill $QPIPE_PID
    exit 1
fi

errcho "Making $REPEAT runs for m = {$MLIST}" 

# make a backup of the qpipe executable so we can repeat the experiment
cp qpipe $SERDIR

function echodbx()
{
    echo echo "$@"
    echo "$@"
}

# Create controlled dbx session
(
    echodbx date
    echodbx attach $QPIPE_PID
    echodbx cont
    echodbx collector store directory $PWD/$DIR
    
    # unshared execution    
    for m in $MLIST; do
	echodbx collector store group $(printf "m%02d-noWS.erg" $m)
	for((i=0; i < REPEAT; i++)); do   
	    echodbx collector enable
	    echodbx cont
	    echodbx collector disable
	done
    done

    # shared execution
    for m in $MLIST; do
	echodbx collector store group $(printf "m%02d-WS.erg" $m)
	for((i=0; i < REPEAT; i++)); do   
	    echodbx collector enable
	    echodbx cont
	    echodbx collector disable
	done
    done
    echodbx exit
) > $DIR/dbx.in

(dbx < $DIR/dbx.in 2>&1) > $DIR/dbx.out &


# find dbx  ($! points to the subshell running it)
find_pid dbx
DBX_PID=$PID
errcho "Started dbx with PID $DBX_PID"

WAIT_STRING="Interactive"
function wait_for_qpipe()
{
    errcho "Waiting for qpipe prompt to return"
    # the prompt doesn't end with a newline...
    while read line; do 
	# make sure we are ready
	errcho "$line"
#	errcho $(echo $line | grep "$WAIT_STRING")
	if echo $line | grep "$WAIT_STRING"; then
	    WAIT_STRING="Throughput"
	    THROUGHPUT=$line
	    # break into dbx so it runs the next set of commands
	    kill -INT $QPIPE_PID
	    return 0
	fi
    done

    # EOF???
    errcho "Unexpected EOF from qpipe. Bailing out"
    kill $DBX_PID
    exit 1
}

# wait for the first qpipe prompt to appear
wait_for_qpipe


function echoqpipe()
{
    errcho "$@"
    echo "$@" >> $DIR/qpipe.in
    echo "$@"
}

# for each number of sharers

# unshared execution
OUTFILE=$DIR/throughput-noWS.dat
echoqpipe sharing global_disable
for m in $MLIST; do
    # repeat 10 times for consistency
    for((i=0; i < REPEAT; i++)); do
	# send the next command to qpipe
	echoqpipe "tpch $QUERY $m 1 0 OS"
	wait_for_qpipe
	echo m$m-$i $(echo $THROUGHPUT | awk '{print $6}') >> $OUTFILE
    done
done

# shared execution
OUTFILE=$DIR/throughput-WS.dat
echoqpipe sharing global_enable
echoqpipe tracer enable TRACE_WORK_SHARING
for m in $MLIST; do
    # repeat 10 times for consistency
    for((i=0; i < REPEAT; i++)); do
	# send the next command to qpipe
	echoqpipe "tpch $QUERY $m 1 0 OS"
	wait_for_qpipe
	echo m$m-$i $(echo $THROUGHPUT | awk '{print $6}') >> $OUTFILE
    done
done

# The last time dbx got an INT it should have run its exit command
errcho "Experiments complete. Exiting..."
echoqpipe "quit"

# pick up the rest of qpipe's output so we don't get a broken pipe
while read line; do errcho $line; done