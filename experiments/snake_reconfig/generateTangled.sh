#!/usr/bin/env bash

# Take a directory with input confiugrations and generate a new directory with
# tangled configurations

# Usage: ./generateTangled.sh inputDirectory sampleCount stepsCount outputDirectory

inputDirectory=$1
sampleCount=$2
stepsCount=$3
outputDirectory=$4

mkdir -p ${outputDirectory}

echo "" > /tmp/commands.txt

for f in ${inputDirectory}/*; do
    for i in $(seq -w ${sampleCount}); do
        output=${outputDirectory}/$(basename $f.$i)
        echo rofi-tangle --steps ${stepsCount} $f $output >> /tmp/commands.txt
    done
done

echo "Running it in parallel:"
parallel < /tmp/commands.txt
