#!/usr/bin/perl

use strict;

unless (@ARGV) {
    my($directory, $filename) = $0 =~ m/(.*\/)(.*)$/;
    print "Usage: $filename <qpipe output files...>\n";
    exit -1;
}



my $DEBUG = 1;
my $DELIMITER_STAT_NAME = 'Description';
my @files = ();
my @FILESTATS_LIST = ();



# Verify that files exist
@files = @ARGV;
foreach my $file (@files) {
    if ( ! -r $file ) {
        print "Error: $file is not a readable file\n";
        exit -1;
    }
}



# Add stats from each file to FILESTATS_LIST
foreach my $file (@files) {
    add_filestats($file);
}
debug_add_filestats();

# Get unique list of descriptions (i.e. 'q1', 'q4', 'sim_mix', etc)
my @UNIQUE_DESC = find_unique_descriptions($FILESTATS_LIST[0]);

foreach my $desc (@UNIQUE_DESC) {
    print "Generating graph for $desc\n";
    generate_desc_graph($desc);
}



sub generate_desc_graph {

    my $desc = shift;
    my @graph_inputs = ();


    foreach my $fs_ent (@FILESTATS_LIST) {

        my $file = $fs_ent->{FILE};
        my $filestats_ref = $fs_ent->{FILESTATS};
        my @filestats = @{$filestats_ref};
        
        # construct graph input filename
        my $input_filename = "$file.$desc";
        $input_filename =~ s/ /_/g;

        open(GRAPH_INPUT, ">$input_filename");
        
        my $input;

        foreach my $fs_ent_fs (@filestats) {

            my $fs_ent_desc = $fs_ent_fs->{"Description"};
            if ( $fs_ent_desc eq $desc ) {
                my $fs_ent_cl = $fs_ent_fs->{"Clients"};
                # trim units from measurement
                my $fs_ent_tp = $fs_ent_fs->{"Throughput"};
                $fs_ent_tp =~ s/ queries\/min//;
                print GRAPH_INPUT "$fs_ent_cl $fs_ent_tp\n";
            }
        }

        $input->{DESCRIPTION} = $desc;
        $input->{FILE} = $file;
        $input->{GRAPH_INPUT_FILE} = $input_filename;
        push(@graph_inputs, $input);

        close(GRAPH_INPUT);
    }

    run_plotter_on_inputs(@graph_inputs);
}



sub run_plotter_on_inputs {

    my @graph_inputs = @_;

    my $input = $graph_inputs[0];
    my $desc = $input->{DESCRIPTION};
    
    open (PLOTTER, "| tee gnuplot_input.txt | gnuplot");

    print PLOTTER "\n reset";
    print PLOTTER "\n set autoscale";
    print PLOTTER "\n set xtic auto";
    print PLOTTER "\n set ytic auto";
    print PLOTTER "\n set logscale xy 2";
    print PLOTTER "\n set title \"$desc\"";
    print PLOTTER "\n set xlabel \"clients\"";
    print PLOTTER "\n set ylabel \"throughput (queries/min)\"";
    
    print PLOTTER "\n set term postscript eps color solid \"Time-Roman\" 18"; 
    print PLOTTER "\n set output \'$desc.eps\'";
    print PLOTTER "\n plot ";

    # We now have a list of graph inputs that we can pass to gnuplot
    my $input_count = 0;
    foreach my $input (@graph_inputs) {

        my $file = $input->{FILE};
        my $graph_input_file = $input->{GRAPH_INPUT_FILE};
        
        if ($input_count > 0) {
            print PLOTTER ", ";
        }

        print PLOTTER " \\\n \"$graph_input_file\" using 1:2 title \"$file\" with line";
        $input_count++;
    }
    
    close(PLOTTER);
}



sub find_unique_descriptions {

    my $rec = shift;
    my @desc_list = ();
    
    my $filestats_ref = $rec->{FILESTATS};
    my @filestats = @{$filestats_ref};

    foreach my $stat (@filestats)
    {
        my $stat_desc = $stat->{'Description'};
        
        my $found = 0;
        foreach my $desc (@desc_list)
        {
            if ($stat_desc eq $desc)
            {
                $found = 1;
                last;
            }
        }
        if (!$found) { push(@desc_list, $stat_desc); }
    }

    return @desc_list;
}



sub add_filestats {

    my $file = shift;
    my @filestats = ();
    my $stathash = ();



    open(FILE, "<$file");

    my $need_to_insert = 0;
    while (<FILE>)
    {
        # Ignore lines not containing ~~~
        if ( ! /~~~/ ){ next; };
        my @tokens = split(/~~~/, $_);
        my $stat   = $tokens[1];

        # chomp \r and \n
        $stat =~ s/\r|\n//g;

        # split into name and value
        $stat =~ /(\S+)\s+=\s+(\S+.*)/;
        my $stat_name  = $1;
        my $stat_value = $2;

        # Ignore lines with no stat name. These lines are fillers.
        if ( $stat_name =~ /^$/ ) { next; }

        # trim whitespace from value
        $stat_value =~ s/^\s+//;
        $stat_value =~ s/\s+$//;

        # We want to collapse multiple lines of output into a single
        # record (basically, an object entry in @filestats). Every
        # time we see a new DELIMITER_STAT_NAME, we can start a new
        # record.
        if ($stat_name eq $DELIMITER_STAT_NAME)
        {
            # starting new record
            if ($need_to_insert)
            {
                push(@filestats, $stathash);
                $stathash = ();
                $need_to_insert = 0;
            }
            if ($DEBUG) { print "Added to filestats. Length now $#filestats\n"; }
        }


        # Initialize the hash 'stathash' with the attribute we just read.
        $stathash->{$stat_name} = $stat_value;
        $need_to_insert = 1;


        if (0 && $DEBUG) {
            print "Inserting into field... [$stat_name] [$stat_value]\n";
        }
    }

    close(FILE);    

    # insert last record
    if ($need_to_insert) {
        push(@filestats, $stathash);
    }                
   
    # We have created an array of statistics for one file. Let's add
    # this array to the global lookup table FILESTATS_LIST.
    my $ins = ();
    my $filestats_ref = \@filestats;
    $ins->{FILE} = $file;
    $ins->{FILESTATS} = $filestats_ref;
    push(@FILESTATS_LIST, $ins);
    print "Inserted $file info into FILESTATS_LIST\n";
}



sub debug_add_filestats {
    if ($DEBUG) {
        
        foreach my $ins (@FILESTATS_LIST)
        {
            my $file = $ins->{FILE};
            my $filestats_ref = $ins->{FILESTATS};
            my @filestats = @{$filestats_ref};
            
            print "file = $file\n";
            print "filestats = $filestats_ref\n";
            print "$#filestats\n";
            
            my $rec_count = 0;
            foreach my $rec (@filestats)
            {
                my $v = $rec->{"Description"};
                print "$rec_count rec->{desciption} = $v\n";
                $rec_count++;            
            }
        }
    }
}
