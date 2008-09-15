#!/bin/bash

DB=shorecmp
echo "connect $DB;"
# first create the tables
db2 connect to $DB
for ((i=0; i < $1; i++)); do
    TABLE="table$(printf '%02d' $i)"
    #ignore errors on drop because the table might not exist yet
    db2 "drop table $TABLE" > /dev/null
    db2 "create table $TABLE (O_ID int, O_D_ID int, O_W_ID int, O_C_ID int,
                     O_ENTRY_D timestamp, O_CARRIER_ID int,
		     O_OL_COUNT int, O_ALL_LOCAL int) in shore_tablespace"
    db2 "create index ${TABLE}_IDX on $TABLE(O_ID, O_D_ID, O_W_ID)"
done
db2 "connect reset"
db2 "terminate"
