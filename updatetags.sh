#!/bin/sh
#
# @file updatetags.sh
#
# @brief Updates the TAGS file

find . -name "*.h" -o -name "*.cpp" | etags -

