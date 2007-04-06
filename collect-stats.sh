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

function errcho()
{
    echo "$@" >&2
}


# sets PID to the desired process, if found, or NULL
function find_pid()
{
    # only allow exact matches at end of line
    PID=$(ps -u$USER | grep " $1$")
    RESULT=$?
    errcho "PID search returned $PID with code $RESULT"
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
\$ collect-stats.sh <query-name> <cores> <output-dir> | ./qpipe > qpipe.out

This script *must* be piped to the input of qpipe to control it
qpipe *must* write its output to 'qpipe.out' for this script to read
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
MLIST="1 2 3 4 6 8 10 12 16 20 24 28 32 36 40"
REPEAT=10

# overrides for testing
MLIST="1 2 3"
REPEAT=2

find_pid qpipe
QPIPE_PID=$PID
errcho "Found qpipe with PID $QPIPE_PID"
errcho "Making $REPEAT runs for m = {$MLIST}" 


# Create dbx commands
(
    echo attach $QPIPE_PID
#    echo set -o '$sigblock'
    echo print '"cont"'
    echo cont
    echo print '"debugger back"'
    #echo collector store directory $DIR
    for m in $MLIST; do
#	echo collector store group $QUERY-n$N-m$m.erg
	for((i=0; i < REPEAT; i++)); do   
#	    echo collector store experiment $QUERY-n$N-m$m-$i.er
#	    echo collector enable
	    echo print '"cont"'
	    echo cont
	    echo print '"debugger back"'
#	    echo collector disable
	done
    done
    echo "exit"
) > dbx.in

# Start dbx
(dbx 2>&1 < dbx.in) > dbx.out &


# find dbx  ($! points to the subshell running it)
find_pid dbx
DBX_PID=$PID
errcho "Started dbx with PID $DBX_PID"
    
function wait_for_qpipe()
{
    errcho "Waiting for qpipe prompt to return"
    # the prompt doesn't end with a newline...
    while read line; do 
	# make sure we are ready
	errcho "$line"
	if echo $line | grep "Throughput" || echo $line | grep "Interactive"; then
	    THROUGHPUT=$(echo $line | grep "Throughput")
	    # make dbx run the next set of commands
#	    sleep 1
#	    kill -INT $DBX_PID
	    kill -INT $QPIPE_PID
#	    sleep 1
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

rm -f $DIR/throughputs.dat

# for each number of sharers
for m in $MLIST; do
    # repeat 10 times for consistency
    for((i=0; i < REPEAT; i++)); do
	# send the next command to qpipe
	echo "tpch $QUERY $m 1 0 OS"
	wait_for_qpipe
	echo $QUERY-n$N-m$m-$i $(echo $THROUGHPUT | awk '{print $6}') >> $DIR/throughputs.dat
    done
done

# by exiting we cause qpipe to quit
# The last time dbx got an INT it was out of commands and exited
errcho "Experiments complete. Exiting..."
echo "quit"

# pick up the rest of qpipe's output so we don't get a broken pipe
while read line; do echo $line; done