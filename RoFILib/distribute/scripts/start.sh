#!/bin/bash

IN_START_MPI="../$1"
IN_TARGET_MPI="../$2"
IN_DIRECTORY="$3/"
IN_COUNT="$3/count"
IN_DICTIONARY="$3/dictionary"
OUT_MPI="run.out"
OUT_POSTPROCESSING="example.out"
LOG="log"


echo "start script"
echo "clean and create build"
if [ -d "./build" ]; then rm -rf build/; fi
mkdir build; cd build; mkdir $IN_DIRECTORY

echo "compile and start distributed algorithm"
cmake ../../.. | tee $LOG &&
  make -j4 | tee $LOG &&
  ./rofi-distribute-preprocessing $IN_START_MPI $IN_TARGET_MPI $IN_DIRECTORY $IN_COUNT $IN_DICTIONARY &&
  ROFI_COUNT=$(head -n 1 $IN_COUNT) &&
  mpiexec --hostfile ../$4 -np $ROFI_COUNT ./distribute/rofi-distribute $IN_DIRECTORY >> $OUT_MPI &&
  ./rofi-distribute-postprocessing $OUT_MPI $IN_DICTIONARY >> $OUT_POSTPROCESSING
echo "finish process"

echo "print files"

echo "----------------------------- LOG -----------------------------"
cat $LOG

echo "--------------------------- OUT_MPI ---------------------------"
cat $OUT_MPI

echo "--------------------------- OUT_POST --------------------------"
cat $OUT_POSTPROCESSING

echo "finish"
