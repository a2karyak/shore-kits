#!/bin/sh

# @file Get configuration
# 
# @brief Displays the basic configuration parameters of the StageTRX environment
#
# @author Ippokratis Pandis (ipandis)

# Gets the RANGE of the warehouses queried
echo "Getting RANGE"
echo " "
greps RANGE include/ | grep define | grep -v "\~"
echo " "


# Gets the optimization level. 
# @note Prints all the optimizer flags. The used is the one which is not commented
echo "Getting OPT_FLAGS"
echo " "
cat configure.in | grep "O3\|O0"
echo " "

# Gets the database size
# @note Check for the whX
echo "Getting DB_SIZE"
echo " "
ls -la database_bdb_tpcc
echo " "
