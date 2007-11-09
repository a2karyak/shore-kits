#!/bin/sh

svn log | ./gnuify-changelog.pl > ChangeLog

automake -a -f
libtoolize
aclocal
autoconf
automake --add-missing
CXX=CC ./configure
