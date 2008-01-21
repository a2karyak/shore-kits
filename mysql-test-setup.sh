#!/bin/bash

DB=ryan1
echo "connect $DB;"
# first create the tables
for ((i=0; i < 32; i++)); do
    TABLE="test$(printf '%02d' $i)"
    COMMAND=$(cat <<EOF
drop table if exists $TABLE;

create table $TABLE (O_ID int, O_D_ID int, O_W_ID int, O_C_ID int,
                     O_ENTRY_D datetime, O_CARRIER_ID int,
		     O_OL_COUNT int, O_ALL_LOCAL int) ENGINE InnoDB;
EOF
)
    echo $COMMAND
done
# then create the stored procs
echo "delimiter //"

for ((i=0; i < 32; i++)); do
    TABLE="test$(printf '%02d' $i)"
    COMMAND=$(cat <<EOF
drop procedure if exists insert_$TABLE // 
create procedure insert_$TABLE(n int)
begin
    declare start, stop datetime;
    declare val int;
    set val=rand();
    set start=sysdate();
    set @s = start;
    prepare insert_stmt from "insert into $TABLE values(?,?+1,?+2,?+3,?,?+4,?+5,?+6)";
    label1:loop
        set n = n-1;
        if n = 0 then leave label1; end if;
	set @x = val+n;
	execute insert_stmt using @x,@x,@x,@x,@s,@x,@x,@x;
    end loop;
    set stop=sysdate();
    select time(stop-start);
    deallocate prepare insert_stmt;
end;
//
EOF
)
#	--insert into $TABLE values(val+n, val+n+1, val+n+2, val+n+3, start, val+n+4, val+n+5, val+n+6) ;

        echo $COMMAND
done
echo "delimiter ;"


