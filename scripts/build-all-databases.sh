#!/bin/bash
#
# Recreates the directories, builds and closes the databases

STAMP=$(date +"%F-%Hh%Mm%Ss")
echo "[$STAMP]" | tee -a building-db

check_dir ()
{
    # Dir name
    DIR=$1

    if [ -d $DIR ] 
        then        
        echo "Deleting $DIR"
        rm $DIR/*
    else
        echo "Creating $DIR"
        mkdir $DIR
    fi
}

load_db ()
{
    # System (baseline,dora,plp)
    SYSTEM=$1

    # Configuration (tm1-16,...)
    CONF=$2

    # Design (normal,mrbtnorm,...)
    DESIGN=$3

    echo "XXX $SYSTEM $CONF $DESIGN XXX"  | tee -a building-db

    STAMP=$(date +"%F-%Hh%Mm%Ss")
    echo "[$STAMP]" | tee -a building-db

    # Check the corresponding log dir
    echo "Checking $SYSTEM-log-$CONF"
    check_dir $SYSTEM-log-$CONF

    if [ -e shore.conf.$SYSTEM ] 
        then
        echo "Using config file shore.conf.$SYSTEM" | tee -a building-db
        else
        echo "!!! POSSIBLE ERROR: using default config file" | tee -a building-db
    fi

    echo "./shore_kits -f shore.conf.$SYSTEM -c $CONF -s $SYSTEM -d $DESIGN -x -r" | tee -a building-db
    #(echo quit | LD_LIBRARY_PATH="/export/home/ipandis/apps/readline/;/usr/sfw/lib/sparcv9" ./shore_kits -f shore.conf.$SYSTEM -c $CONF -s $SYSTEM -d $DESIGN -x -r ) 2>&1 | tee -a building-db
}


### Make sure that the databases directory exists
check_dir databases


### The various databases that will be created
load_db baseline tm1-1 normal

# load_db baseline tm1-16 normal
# load_db baseline tm1-64 normal
# load_db baseline tpcb-20 normal
# load_db baseline tpcb-100 normal
# load_db baseline tpcc-20 normal
# load_db baseline tpcc-100 normal

# load_db dora tm1-16 normal
# load_db dora tm1-64 normal
# load_db dora tpcb-20 normal
# load_db dora tpcb-100 normal
# load_db dora tpcc-20 normal
# load_db dora tpcc-100 normal


# Instead of plp we use dora-mrbtnorm
# load_db plp tm1-16 mrbtnorm
# load_db plp tm1-64 mrbtnorm
# load_db plp tpcb-20 mrbtnorm
# load_db plp tpcb-100 mrbtnorm
# load_db plp tpcc-20 mrbtnorm
# load_db plp tpcc-100 mrbtnorm

# load_db dora tm1-16 mrbtnorm
# load_db dora tm1-64 mrbtnorm
# load_db dora tpcb-20 mrbtnorm
# load_db dora tpcb-100 mrbtnorm
# load_db dora tpcc-20 mrbtnorm
# load_db dora tpcc-100 mrbtnorm

