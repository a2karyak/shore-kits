#!/usr/bin/perl

# dbgen-generate-files.pl
#
# Generates the files by calling dbgen_tpcc

use strict;

my $TPCC_TABLE_CMD = "./dbgen_tpcc";
my $TPCC_TABLE_DIRECTORY = "/usr3/ipandis/QPIPE_TPCC_DATA/ibm";
my @TPCC_TABLE_ID = ( 3, 4, 5, 6, 7, 8, 9, 11);

my $ls_cmd = "ls -la";

foreach (@TPCC_TABLE_ID) {

    my $table = $_;

    my $command = "$TPCC_TABLE_CMD -t $table -f $TPCC_TABLE_DIRECTORY/$table.dat";

    if ($table == 9) {
        $command = "$TPCC_TABLE_CMD -t $table -f1 $TPCC_TABLE_DIRECTORY/9.dat -f2 $TPCC_TABLE_DIRECTORY/10.dat";
    }

    print $command;
    print "\n";

    system($command) == 0
        or die "system() failed to run ($command)";

    print "$TPCC_TABLE_DIRECTORY/$table.dat created\n";
    $ls_cmd = "ls -la $TPCC_TABLE_DIRECTORY/$table.dat";

#   Execute ls command without caring about result
    system($ls_cmd);

}

$ls_cmd = "ls -la $TPCC_TABLE_DIRECTORY";
system($ls_cmd);


