#!/bin/bash

#@file:   scripts/do-remaining-runs
#@brief:  Remaining runs !!
#@author: Ippokratis Pandis

echo " "
echo "POWER RUNS"
echo " "

#### DONE
# echo "DORA-TM1-{UpdSubData,CallFwdMix,TM1Mix} POWER"
# SEQ=(224 226 200) ; for trx in ${SEQ[@]}; do ./scripts/powerwrapper.sh EXP tm1-100-dlog10-nl 1 80 $trx 30 3 300; done

#### DONE
# echo "BASE-TM1-{UpdSubData,CallFwdMix,TM1Mix} POWER"
# SEQ=(24 26 20) ; for trx in ${SEQ[@]}; do ./scripts/powerwrapper.sh EXP tm1-100-dlog10 1 80 $trx 30 3 300; done


echo " "
echo "DBX RUNS"
echo " "



#### DONE
# echo "DORA-TM1-{UpdSubData,CallFwdMix,TM1Mix} DBX"
# SEQ=(224) ; for trx in ${SEQ[@]}; do ./scripts/dbxrunwrapper.sh EXPDBX tm1-100-dlog10-nl $trx ./scripts/dorarun.sh 100 30 3 $trx 400; done

echo "DORA-TM1-{CallFwdMix,TM1Mix} DBX"
SEQ=(226 200) ; for trx in ${SEQ[@]}; do ./scripts/dbxrunwrapper.sh EXPDBX tm1-100-dlog10-nl $trx ./scripts/dorarun.sh 100 30 3 $trx 400; done

echo "BASE-TM1-{UpdSubData,CallFwdMix,TM1Mix} POWER"
SEQ=(24 26 20) ; for trx in ${SEQ[@]}; do ./scripts/dbxrunwrapper.sh EXPDBX tm1-100-dlog10 $trx ./scripts/dorarun.sh 100 30 3 $trx 400; done

echo "BASE-TPCC-{Payment,NewOrder,LitteMix,Mix} POWER"
SEQ=(2 1 9 0) ; for trx in ${SEQ[@]}; do ./scripts/dbxrunwrapper.sh EXPDBX tpcc-100-dlog10 $trx ./scripts/dorarun.sh 100 30 3 $trx 1500; done



echo "BASE-TPCC-{Payment,NewOrder,LitteMix,Mix} POWER"
SEQ=(2 1 9 0) ; for trx in ${SEQ[@]}; do ./scripts/powerwrapper.sh EXP tpcc-100-dlog10 1 80 $trx 30 3 1200; done

