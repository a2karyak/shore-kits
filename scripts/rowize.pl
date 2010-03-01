#!/opt/csw/bin/perl

#@file:   rowize.pl
#@brief:  Gets the results of a measurement and puts all the collected values for 
#         each run to the same row 
#@author: Ippokratis Pandis

use strict;
use Getopt::Std;

use Statistics::Descriptive;

# fn: Print usage and exit
sub print_usage_exit 
{
    print "USAGE:    $0 -f <INFILE>\n";
    print "OPTIONS:  -f - Input file\n"; 

    exit();
}

# Options
my $infile;
my $outfile;
my $outdir = ".";

# Parse command line
my %options;
getopts("hf:d:",\%options);

# If -h print usage only
print_usage_exit() if $options{h};

$infile = $options{f} ? $options{f} : print_usage_exit();
$outfile = "row.$infile";

# Variables
my $runs=0;
my $mvalues=0;

my $line;
my $value;
my $valueCnt=0;
my $tag;
my $clients;
my $entries;

# The four arrrays we will keep the data
my @theTags;
my @theClients;
my @theEntriesPerClient;
my @theValues;

#open(infile) or die("Could not open input file $infile.");
#open(infile);

open INFILE, "<", "$infile" or die "Cannot open file $infile.";

# Read input file
# Count the number of different measured types (mvalues)
# Count the number of runs (runs)
foreach $line (<INFILE>) {

    ($value, $tag, $clients) = split(' ',$line);

# Check if it is a new tag (measured type)
    if (lc($value) eq "++")
    {
        printf 'TAG -> %d. %s', $mvalues, $tag;
        printf "\n";
        $mvalues=$mvalues+1;
        push(@theTags,$tag);
        if ($mvalues == 2)
        {
            push(@theEntriesPerClient,$entries);
        }
        $entries=0;
    }
    
# It will count all the lines whose tag is "measure" and we are still in
# the first tag
# It will zero the number of entries per measurem
    if (($mvalues == 1) && (lc($tag) eq "measure"))
    {
        printf 'CL  -> %d. %d', $runs, $clients;
        printf "\n";
        $runs=$runs+1;
        push(@theClients,$clients);

        printf 'ENT -> %d. %d', $runs-1, $entries;
        printf "\n";
        push(@theEntriesPerClient,$entries);
        $entries=0;
    }

# It will count the number of entries per measurement
    if (($mvalues == 1) && (lc($tag) ne "measure") && (lc($value) ne "++"))
    {
        printf 'ENT -> %d. %d', $clients, $entries;
        printf "\n";
        $entries=$entries+1;
    }

# It will store all the values (non tags or measure)
    if ((lc($tag) ne "measure") && (lc($value) ne "++"))
    {
        printf 'VAL -> %d. %d', $valueCnt, $value;
        printf "\n";
        $valueCnt=$valueCnt+1;
        push(@theValues,$value);
    }
}

# At the end print everything
print "\n";

#add at the head of theTags array the Clients tag
#unshift(@theTags,"Clients");
print "TheTags:\n";
print "@theTags";
print "\n";

print "TheClients\n";
print "@theClients";
print "\n";

#remove the first element of theEntriesPerClient array
shift(@theEntriesPerClient);
print "TheEntriesPerClient\n";
print "@theEntriesPerClient";
print "\n";

print "TheValues\n";
print "@theValues";
print "\n";

print "\n-------------------------------------\n";

# Calculate the totalEntries
my $totalEntries += $_ foreach @theEntriesPerClient;

my $sumEntries=0;
my $offsetClient=0;
my $offsetValue=0;
my $tagCnt=0;

my $stat;
my $stMean=0;
my $stStd=0;

foreach $entries (@theEntriesPerClient) 
{
    printf $theClients[$offsetClient] . " ";

    foreach $tag (@theTags)
    {
        $offsetValue=($totalEntries*$tagCnt)+$sumEntries;
        $clients=0;
        $stat = Statistics::Descriptive::Full->new();
        while ($clients < $entries)
        {         
            $value=$theValues[$offsetValue];
                       
            $stat->add_data($value);

            $clients=$clients+1;
            $offsetValue=$offsetValue+1;
        }
        
        $stMean=$stat->mean();
        $stStd=$stat->standard_deviation();

        printf("T=%s M=%.2f StD=%.2f | ",$tag,$stMean,$stStd);        
        
        $tagCnt=$tagCnt+1;
    }
    
    $sumEntries=$sumEntries+$entries;
    $offsetClient=$offsetClient+1;
    print "\n\n";
}



print "\n-------------------------------------\n";    
