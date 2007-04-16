#!/usr/bin/perl

use strict;

my @workloads = ('./sim_mix_commands.txt', './q16_commands.txt');

my $OUTPUT_DIR = "./output/lomond_04.15.2007";
my $program = './qpipe';
#my $program = "/export/home/ngm/.user_env/bin/monitor_echo";
$| = 1;

my @COMMON_CONFIG_VARIABLES = (
    'GLOBAL_OSP',
    'USE_DETERMINISTIC_PREDICATES'
    );

my @FLUSH_CONFIG_VARIABLES =  (
    'WAIT_FOR_UNSHARED_TUPLE_FIFOS_TO_DRAIN',
    'USE_DIRECT_IO'
    );


die "$program not an executable program" if (! -x "$program" );


my @command_list = ();


foreach my $w (@workloads) {



    # Phase 1: Do not push anything to disk
    if (0) {
        my $common_size  = 1 + $#COMMON_CONFIG_VARIABLES;
        my $common_limit = 2 ** $common_size;
        foreach my $common_config ( 0 .. ($common_limit-1) ) {
            
            my $path_ext = $OUTPUT_DIR."/"."${w}_FLUSH_OFF";
            my @commands = ();

            # go through bits
            foreach my $bit_index (0 .. ($common_size-1)) {
                
                my $bit = ($common_config >> $bit_index) & 1;
                my $var = $COMMON_CONFIG_VARIABLES[$bit_index];
                if ($bit) {
                    $path_ext = "${path_ext}_${var}_ON";
                    push(@commands, "config enable $var");
                }
                else {
                    $path_ext = "${path_ext}_${var}_OFF";
                    push(@commands, "config disable $var");
                }
            }

            # run workload commands
            my $wl_command;
            open(WORKLOAD_COMMANDS, $w);
            while ($wl_command = <WORKLOAD_COMMANDS>) {
                push(@commands, $wl_command);
            }
            close(WORKLOAD_COMMANDS);

            # now we have constructed @commands
            open(PROGRAM, " | $program | tee $path_ext");
            foreach my $command (@commands) {
                print PROGRAM "$command \n";
            }
        }

    } # endof Phase 1



    # Phase 2: Enable disk flushing
    if (1) {
        
        my @all_config = (@COMMON_CONFIG_VARIABLES, @FLUSH_CONFIG_VARIABLES);
        my $all_size = 1 + $#all_config;
        my $all_limit = 2 ** $all_size;
        foreach my $all_config ( 0 .. ($all_limit-1)) {
            
            my $path_ext = $OUTPUT_DIR."/"."${w}_FLUSH_ON";
            my @commands = ();

            # go through bits
            foreach my $bit_index (0 .. ($all_size-1)) {
                
                my $bit = ($all_config >> $bit_index) & 1;
                my $var = $all_config[$bit_index];
                if ($bit) {
                    $path_ext = "${path_ext}_${var}_ON";
                    push(@commands, "config enable $var");
                }
                else {
                    $path_ext = "${path_ext}_${var}_OFF";
                    push(@commands, "config disable $var");
                }
            }

            # run workload commands
            my $wl_command;
            open(WORKLOAD_COMMANDS, $w);
            while ($wl_command = <WORKLOAD_COMMANDS>) {
                push(@commands, $wl_command);
            }
            close(WORKLOAD_COMMANDS);

            # now we have constructed @commands
            open(PROGRAM, " | $program | tee $path_ext");
            print PROGRAM "config enable FLUSH_TO_DISK_ON_FULL\n";
            foreach my $command (@commands) {
                print PROGRAM "$command \n";
            }
        }

    } # endof Phase 2

}
