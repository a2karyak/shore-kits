#!/bin/sh

automake -a -f
libtoolize -f
aclocal
autoconf -f
automake --add-missing --force

