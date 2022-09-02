#!/usr/bin/env bash

set -e

SAMPLES=50
OUTDIR=$1
STEPS=$2

mkdir -p ${OUTDIR}

counter=1
total=$(ls ${ROFI_ROOT}/data/configurations/snake/ | wc -l)

jobLimit=8
currentJobs="\j"

for source in ${ROFI_ROOT}/data/configurations/snake/*.rofi; do
    for i in $(eval echo "{1..${SAMPLES}}"); do
        while (( ${currentJobs@P} >= ${jobLimit} )); do
            wait -n
        done
        echo Running ${counter}/$((${total} * ${SAMPLES}))
        rofi-tangle --steps=${STEPS} ${source} ${OUTDIR}/${source%.rofi}-${i}-${STEPS}.rofi &
        counter=$(($counter+1));
    done
done
