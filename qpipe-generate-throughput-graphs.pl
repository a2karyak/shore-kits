#!/usr/bin/perl

use strict;

unless ($ARGV) {
    my($directory, $filename) = $0 =~ m/(.*\/)(.*)$/;
    print "Usage: $filename <qpipe output files...>\n";
    exit -1;
}

my $need_newline = 0;
my $curr_description;
while (<STDIN>)
{
    # Ignore lines not containing ~~~
    if ( ! /~~~/ ){ next; };
    my @tokens = split(/~~~/, $_);
    my $stat   = $tokens[1];
    
    $stat =~ /\s+(\S+)\s+=\s+(\S+)/;
    my $stat_name  = $1;
    my $stat_value = $2;
    
    # Ignore lines with no stat name. These lines are fillers.
    if ( $stat_name =~ /^$/ ) { next; }


    # We want to collapse multiple lines of output into a single CSV
    # record (basically, a line in a CSV file). Every time we see a
    # new "Description", we can start a new record.
    if ( $stat_name =~ /Description/ ) {
        # starting new record
        if ($need_newline) {
            print "\n";
            $need_newline = 0;
        }
        $curr_description = $stat_value;
    }

    # Finally, we only want to print the records that we are
    # interested in.
    if (($curr_description eq $desc)
        && !($stat_name eq "Description")) {
        print "$stat_value ";
        $need_newline = 1;
    }
}

print "\n";
