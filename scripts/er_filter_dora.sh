 #!/bin/bash

#@file:   er_filter_dora.sh 
#@brief:  Post-processing script for the output of dbx, for DORA 
#
#@author: Ryan Johnson
#@author: Ippokratis Pandis

in_stack()
{
    echo '(fname("'"$1"'") some in stack)'
}
is_leaf()
{
    echo '(leaf in fname("'"$1"'"))'
}
parenthize()
{
    echo '('"$*"')'
}

# All server side + begin_xct
BEGIN=$(in_stack ".*begin_xct.*")
SERVE=$(in_stack ".*_serve_action.*")

BASE=$(parenthize "$BEGIN || $SERVE")


# Components in graphs
LM=$(in_stack ".*lock_m.*")
LOGM=$(in_stack ".*log.*")


# Contention
ATOMIC=$(is_leaf "atomic[^:]*")
PPMCS=$(is_leaf "ppmcs.*acquire.*")
OCC_RWLOCK=$(is_leaf "occ_rwlock.*")
SPIN=$(is_leaf ".*lock.*spin.*")
MUTEX=$(is_leaf "mutex.*lock.*")

CONTENTION=$(parenthize "$ATOMIC || $PPMCS || $SPIN || $OCC_RWLOCK || $MUTEX")

# DORA
DORA=$(is_leaf "dora.*")


echo " "
echo " "
echo " "
echo " "
date +"%r"
echo "$1"
echo " "
echo " "
echo " "
echo " "


# echo "$BASE"
# echo "$BASE && $CONTENTION"
# echo "$BASE && $LM"
# echo "$BASE && $LM && $CONTENTION"
# echo "$BASE && $LOGM"
# echo "$BASE && $LOGM && $CONTENTION"


er_print -limit 5 \
    -filters "$BASE" -functions \
    -filters "$BASE && $CONTENTION" -functions \
    -filters "$BASE && $LM" -functions \
    -filters "$BASE && $LM && $CONTENTION" -functions \
    -filters "$BASE && $LOGM" -functions \
    -filters "$BASE && $LOGM && $CONTENTION" -functions \
    $@