#!/usr/bin/perl

use strict;

my @files = ('qpipe_no_flush',
             'qpipe_flush_use_direct',
             'qpipe_flush_use_fscache');

foreach my $file (@files) {
    system("./$file < commands.txt | tee ${file}_output.txt");
}
