#!/bin/sh
#
# @file:   run-sanity.sh
#
# @brief:  Runs a bunch of sanity checks
#
# @todo:   To be moved to make check
#
# @author: Ippokratis Pandis (ipandis@us.ibm.com)

./shore_kits -r -c tm1-16 -s dora -d normal -x -i scripts/sanity-dora.in
