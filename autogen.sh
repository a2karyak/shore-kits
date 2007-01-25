#!/bin/sh

svn log | ./gnuify-changelog.pl > ChangeLog
libtoolize
aclocal
autoconf
automake
./configure
