#!/bin/sh
#
# @file do_scale_cls.sh
#
# @brief Increases the number of clients that submit
# 100K trxs

echo "Thr *** CL=1 Q=100K WH=1" | tee sng_thr.txt
./tests/inmem_payment_single_thr 1 100000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

echo "Thr *** CL=2 Q=100K WH=1" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 2 100000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

echo "Thr *** CL=3 Q=100K WH=1" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 3 100000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

echo "Thr *** CL=4 Q=100K WH=1" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 4 100000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

echo "Thr *** CL=5 Q=100K WH=1" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 5 100000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

echo "Thr *** CL=6 Q=100K WH=1" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 6 100000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

echo "Thr *** CL=8 Q=100K WH=1" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 8 100000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

echo "Thr *** CL=10 Q=100K WH=1" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 10 100000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt
