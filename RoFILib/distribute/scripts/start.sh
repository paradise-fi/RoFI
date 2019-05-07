#!/bin/bash

IN_START_MPI="../example.in"
IN_TARGET_MPI="../exampleTrg.in"
OUT_MPI='run.out'
OUT_POSTPROCESSING='example.out'
LOG='log'


echo "start script"
echo "clean and create build"
if [ -d "./build" ]; then rm -rf build/; fi
mkdir build; cd build

echo "compile and start distributed algorithm"
ROFI_COUNT=3
cmake ../../.. | tee $LOG &&  
  make | tee $LOG &&
  mpiexec -np $ROFI_COUNT ./distribute/rofi-distribute $IN_START_MPI $IN_TARGET_MPI >> $OUT_MPI &&
  ./rofi-distribute-postprocessing $OUT_MPI >> $OUT_POSTPROCESSING
echo "finish process"

echo "print files"

echo "----------------------------- LOG -----------------------------"
cat $LOG

echo "--------------------------- OUT_MPI ---------------------------"
cat $OUT_MPI

echo "--------------------------- OUT_POST --------------------------"
cat $OUT_POSTPROCESSING

echo "finish"
