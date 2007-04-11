#!/usr/bin/perl

use strict;

my @files = ('qpipe_no_flush',
             'qpipe_flush_use_fscache',
             'qpipe_flush_use_direct',
             'qpipe_no_flush_osp_none',
             'qpipe_flush_use_fscache_osp_none',
             'qpipe_flush_use_direct_osp_none');

foreach my $file (@files) {
    die "$file not an executable program" if (! -x "./$file" );
}

foreach my $file (@files) {
    system("./$file < commands.txt | tee output/${file}_output.txt");
}
