#!/bin/bash

IN_MPI="../input.txt"
OUT_MPI='runOutput.txt'
OUT_POSTPROCESSING='result.txt'
LOG='log'


echo "start script"
echo "clean and create build"
if [ -d "./build" ]; then rm -rf build/; fi
mkdir build; cd build

echo "compile and start distributed algorithm"
ROFI_COUNT=$(head -n 1 $IN_MPI)
cmake ../../.. | tee $LOG && make | tee $LOG && mpiexec -np $ROFI_COUNT ./distribute/rofi-distribute $IN_MPI >> $OUT_MPI
./rofi-distribute-postprocessing $OUT_MPI >> $OUT_POSTPROCESSING
echo "finish process"

echo "print files"

echo "--------------------------- LOG ---------------------------"
cat $LOG

echo "--------------------------- OUT_MPI ---------------------------"
cat $OUT_MPI

echo "--------------------------- OUT_POST ---------------------------"
cat $OUT_POSTPROCESSING

echo "finish"
