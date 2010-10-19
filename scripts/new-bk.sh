#!/bin/bash

#
# Filtering strategy:
#
# Each pass peels off some samples, categorizing them and leaving the
# rest behind for further passes. This ensures no double-counting is
# possible.
#

DUMP_SPLIT_DAT="$*"
[ -n "$DUMP_SPLIT_DAT" ] && echo "Dumping .dat file for each category" >&2
function any() {
    sep='('
    while [ "x$1" != "x" ]; do
	echo -n "$sep$1"
	sep='|'
	shift
    done
    echo ')'
}

function blame() {
    echo -n "{ "
    [ -n "$DUMP_SPLIT_DAT" ] && echo -n "print \$0 > \"test-$1.dat\"; "
    echo -n "totals[\"$1\"]+=\$1; next }"
}

gawk -f <(cat <<EOF
BEGIN { $(echo 'totals["'{misc,ignore,catalog,latch,lock,bpool,log,xct_mgt,btree,heap,ssm,kits,misc}'"]=0; ') }

# grand total
{ totals["total"]+=\$1 }

# certain classes of sleep time are unimportant
/ pthread_cond_wait $(any __1cFshoreNbase_client_tIrun_xcts __1cEbf_mK_clean_buf __1cOchkpt_thread_tDrun __1cUpage_writer_thread_tDrun __1cFshoreJsrmwqueue __1cIlog_coreMflush_daemon __1cFshoreTshell_await_clients)/ $(blame ignore)
/ pthread_cond_timedwait __1cTsunos_procmonitor_t/ $(blame ignore)

# snag CATALOG stuff first because we want to include any
# latching/locking it causes
/ __1cFdir_m/ $(blame catalog)

# latching
/ __1cHlatch_t/ $(blame latch)

# locking
/ (__1cGlock_m|__1cLlock_core_m)/ $(blame lock)

# bpool
/ $(any __1cEbf_m __1cJbf_core_m __1cUpage_writer_thread_t __1cTbf_cleaner_thread_t)/ $(blame bpool)

# logging
/ $(any log_ __1cFlog_m __1cIlog_core  __1cFshoreJflusher_t)/ $(blame log)

# xct mgt
/ __1cFxct_t/ $(blame xct_mgt)

# btree
/ $(any __1cHbtree_m __1cKbtree_impl)/ $(blame btree)

# heap
/ $(any __1cGfile_m __1cFpin_i)/ $(blame heap)

# SSM
/ $(any __1cTsunos_procmonitor_t __1cEss_m __1cGpage_pF __1cJw_error_t)/ $(blame ssm)

# kits
/ __1cFshore/ $(blame kits)

# leftovers
$(blame misc)

END { for (c in totals) { print c,totals[c] } }
EOF
)

