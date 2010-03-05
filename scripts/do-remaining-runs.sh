#!/bin/bash

#@file:   scripts/do-remaining-runs
#@brief:  Remaining runs !!
#@author: Ippokratis Pandis


# echo " "
# echo "DBX RUNS"
# echo " "

# echo " "
# echo "TM-1-DORA"
# echo " "

# echo "DORA-TM1-{UpdSubData,CallFwdMix,TM1Mix} DBX"
# SEQ=(224 226 200) ; for trx in ${SEQ[@]}; do ./scripts/dbxrunwrapper.sh EXPDBX tm1-100-dlog10-nl $trx ./scripts/dorarun.sh 100 30 3 $trx 400 large; done


# echo " "
# echo "TM-1-BASE"
# echo " "

# echo "BASE-TM1-{UpdSubData,CallFwdMix,TM1Mix} DBX"
# SEQ=(24 26 20) ; for trx in ${SEQ[@]}; do ./scripts/dbxrunwrapper.sh EXPDBX tm1-100-dlog10 $trx ./scripts/dorarun.sh 100 30 3 $trx 400 large; done


# echo " "
# echo "TPCB-DORA"
# echo " "

# echo "DORA-TPCB DBX"
# SEQ=(331) ; for trx in ${SEQ[@]}; do ./scripts/dbxrunwrapper.sh EXPDBX tpcb-60-dlog10-nl $trx ./scripts/dorarun.sh 100 30 3 $trx 400 large; done


# echo " "
# echo "TPCB-BASE"
# echo " "

# echo "BASE-TPCB DBX"
# SEQ=(31) ; for trx in ${SEQ[@]}; do ./scripts/dbxrunwrapper.sh EXPDBX tpcb-100-dlog10 $trx ./scripts/dorarun.sh 100 30 3 $trx 400 large; done



# echo " "
# echo "TPCC-BASE"
# echo " "

# echo "BASE-TPCC-{Payment,NewOrder,LitteMix,Mix} DBX"
# SEQ=(2 1 9 0) ; for trx in ${SEQ[@]}; do ./scripts/dbxrunwrapper.sh EXPDBX tpcc-100-dlog10 $trx ./scripts/dorarun.sh 100 30 3 $trx 1500large; done



echo " "
echo "POWER RUNS"
echo " "


echo " "
echo "TM1-DORA"
echo " "

# echo "DORA-TM1-{UpdLocation} POWER - ExtraLarge"
# SEQ=(225) ; for trx in ${SEQ[@]}; do ./scripts/powerwrapper.sh EXP tm1-100-dlog10-nl 1 100 $trx 30 3 300 extralarge; done

# echo "DORA-TM1-{GetSubData,GetAccData,CallFwdMix} POWER - Large"
# SEQ=(221 223 226) ; for trx in ${SEQ[@]}; do ./scripts/powerwrapper.sh EXP tm1-100-dlog10-nl 1 80 $trx 30 3 300 large; done

# echo "DORA-TM1-{UpdSubData} POWER - MEDIUM"
# SEQ=(224) ; for trx in ${SEQ[@]}; do ./scripts/powerwrapper.sh EXP tm1-100-dlog10-nl 1 80 $trx 30 3 300 medium; done

# echo "DORA-TM1-{TM1Mix,GetNewDest} POWER - SMALL"
# SEQ=(222 200) ; for trx in ${SEQ[@]}; do ./scripts/powerwrapper.sh EXP tm1-100-dlog10-nl 1 80 $trx 30 3 300 small; done
#SEQ=(200) ; for trx in ${SEQ[@]}; do ./scripts/powerwrapper.sh EXP tm1-100-dlog10-nl 1 80 $trx 30 3 300 small; done

echo " "
echo "TM1-BASE"
echo " "

# echo "BASE-TM1-{ALL} POWER"
# SEQ=(20 21 22 23 24 25 26) ; for trx in ${SEQ[@]}; do ./scripts/powerwrapper.sh EXP tm1-100-dlog10 1 80 $trx 30 3 300 large; done


echo " "
echo "TPCB-DORA"
echo " "

# echo "DORA-TPCB POWER"
# SEQ=(331) ; for trx in ${SEQ[@]}; do ./scripts/powerwrapper.sh EXP tpcb-60-dlog10-nl 1 80 $trx 30 3 300 small; done


echo " "
echo "TPCB-BASE"
echo " "

echo "BASE-TPCB POWER"
SEQ=(31) ; for trx in ${SEQ[@]}; do ./scripts/powerwrapper.sh EXP tpcb-100-dlog10 1 80 $trx 30 3 300 large; done


echo " "
echo "TPCC-BASE"
echo " "

echo "BASE-TPCC-{Payment,NewOrder,LitteMix,Mix} POWER"
SEQ=(2 1 9 0) ; for trx in ${SEQ[@]}; do ./scripts/powerwrapper.sh EXP tpcc-100-dlog10 1 80 $trx 30 3 1200 large; done
