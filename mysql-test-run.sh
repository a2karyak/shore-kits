#!/bin/bash
if [ $# -ne 1 ]; then
    cat <<EOF
Usage: ./mysql-test-run.sh <num-threads>
	Tests how fast and scalable mysql row inserts are.
	Each thread inserts 30k records into its own table

	You must have run ./mysql-test-setup.sh at some point before this.
EOF
    exit -1
fi

DB=ryan1

for((i=0; i < $1; i++)); do
    TABLE="test$(printf '%02d' $i)"
    COMMAND=$(cat <<EOF
connect $DB;
set autocommit=0;
call insert_$TABLE(30000);
commit;
EOF
)
    (echo $COMMAND | mysql) &
done

