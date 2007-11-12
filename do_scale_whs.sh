#!/bin/sh
#
# @file do_scale_whs.sh
#
# @brief Increases the number of queried WHs from 1 to 10 
# for 32 Clients submitting 50K trxs.

echo "Thr *** CL=32 Q=50K WH=1" | tee sng_thr.txt
./tests/inmem_payment_single_thr 32 50000 1 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

echo "Thr *** CL=32 Q=50K WH=2" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 32 50000 2 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

echo "Thr *** CL=32 Q=50K WH=3" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 32 50000 3 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

echo "Thr *** CL=32 Q=50K WH=4" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 32 50000 4 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

echo "Thr *** CL=32 Q=50K WH=5" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 32 50000 5 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

echo "Thr *** CL=32 Q=50K WH=6" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 32 50000 6 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

echo "Thr *** CL=32 Q=50K WH=7" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 32 50000 7 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

echo "Thr *** CL=32 Q=50K WH=8" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 32 50000 8 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

echo "Thr *** CL=32 Q=50K WH=9" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 32 50000 9 | tee -a sng_thr.txt
echo "Thr ***" | tee -a sng_thr.txt

echo "Thr *** CL=32 Q=50K WH=10" | tee -a sng_thr.txt
./tests/inmem_payment_single_thr 32 50000 10 | tee -a sng_thr.txt


