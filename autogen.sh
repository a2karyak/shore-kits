#!/bin/sh

automake -a -f
libtoolize
aclocal
autoconf
automake --add-missing
