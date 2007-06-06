#!/usr/bin/perl

# dbgen-generate-files.pl
#
# Generates the files by calling dbgen_tpcc

use strict;
use Getopt::Std;
use POSIX;


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

foreach (@TPCC_TABLE_ID) {

    my $table = $_;

    my $command = "$TPCC_TABLE_CMD -t $table -r 1 $wh_count -f $TPCC_TABLE_DIRECTORY/$table.dat";

    if ($table == 9) {
        $command = "$TPCC_TABLE_CMD -t $table -r 1 $wh_count -f1 $TPCC_TABLE_DIRECTORY/9.dat -f2 $TPCC_TABLE_DIRECTORY/10.dat";
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


