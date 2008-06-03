#!/usr/bin/perl

# dbgen-generate-files.pl
#
# Generates the files by calling dbgen_tpcc

# !!! CAUTION !!!
# Uses the new Threading feature of Perl 


use strict;
use Getopt::Std;
use POSIX;
use Thread;


# fn: Print usage and exit
sub print_usage_exit {

    print "USAGE:    $0 -w WH_COUNT -d OUT_DIR\n";
    print "OPTIONS:  -w WH_COUNT - Specify the number of warehouses (TPC-C scale factor)\n";
    print "          -d OUT_DIR  - Specify the output directory\n";

    exit();
}


# Variables
my $table_directory;
my $wh_count;
my $ls_cmd = "ls -la";

# Default values
my $TPCC_WH_COUNT = 10;
my $TPCC_TABLE_CMD = "./dbgen_tpcc";
my $TPCC_TABLE_DIRECTORY = "/usr3/ipandis/QPIPE_TPCC_DATA/ibm";
my @TPCC_TABLE_ID = ( 3, 4, 5, 6, 7, 8, 9, 11);



# Parse command line
my %options;
getopts("hw:d:", \%options);

# Print usage only
print_usage_exit() if $options{h};

# Parse options
$wh_count        = $options{w} ? $options{w} : $TPCC_WH_COUNT;
$table_directory = $options{d} ? $options{d} : $TPCC_TABLE_DIRECTORY;


# Checked parsed values
if ($wh_count < 1) {
    die "invalid warehouse value = $wh_count";
}

my @Threads;
my $iCounter = 0;


# Spawn threads
foreach (@TPCC_TABLE_ID) {

    my $table = $_;

    my $command = "$TPCC_TABLE_CMD -t $table -r 1 $wh_count -f $table_directory/$table.dat";

    if ($table == 9) {
        $command = "$TPCC_TABLE_CMD -t $table -r 1 $wh_count -f1 $table_directory/9.dat -f2 $table_directory/10.dat";
    }

#     print $command;
#     print "\n";

    # do the actual thread spawn
    $Threads[$iCounter] = new Thread \&generate_table, $iCounter, $command;
    $iCounter = $iCounter + 1;
}

# Join threads
$iCounter = 0;
foreach (@TPCC_TABLE_ID) {

    my @ReturnData = $Threads[$iCounter]->join;
    print "Thread $iCounter returned @ReturnData\n";
    $iCounter = $iCounter + 1;
}

print "All threads joined...\n";

#   Execute ls command without caring about result
$ls_cmd = "ls -la $table_directory";
system($ls_cmd);


sub generate_table {

    my $id = $_[0];
    my $gen_cmd = $_[1];

    print "$id => $gen_cmd\n";

    system($gen_cmd) == 0
        or die "system() failed to run ($gen_cmd)";

#     print "$table_directory/$table.dat created\n";
#     $ls_cmd = "ls -la $table_directory/$table.dat";

    return $id;
}

