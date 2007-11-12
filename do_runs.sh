#!/bin/sh
#
# @file do_runs
# 
# @brief For CL=1 to 256 (powers of 2), each submitting 25K trxs
# we increase the number of queried WHs from 1 to 5.

./tests/inmem_payment_single_thr 1 25000 1 | tee sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 2 25000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 4 25000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 8 25000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 16 25000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 32 25000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 64 25000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 128 25000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 256 25000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

./tests/inmem_payment_single_thr 1 25000 2 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 2 25000 2 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 4 25000 2 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 8 25000 2 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 16 25000 2 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 32 25000 2 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 64 25000 2 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 128 25000 2 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 256 25000 2 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

./tests/inmem_payment_single_thr 1 25000 3 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 2 25000 3 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 4 25000 3 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 8 25000 3 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 16 25000 3 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 32 25000 3 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 64 25000 3 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 128 25000 3 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 256 25000 3 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

./tests/inmem_payment_single_thr 1 25000 4 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 2 25000 4 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 4 25000 4 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 8 25000 4 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 16 25000 4 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 32 25000 4 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 64 25000 4 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 128 25000 4 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 256 25000 4 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt


./tests/inmem_payment_single_thr 1 25000 5 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 2 25000 5 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 4 25000 5 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 8 25000 5 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 16 25000 5 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 32 25000 5 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 64 25000 5 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 128 25000 5 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 256 25000 5 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt


