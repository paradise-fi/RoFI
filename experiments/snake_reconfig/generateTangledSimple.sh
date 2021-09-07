#!/usr/bin/env bash

# Take an input confiugrations and generate a new directory with
# tangled configurations

# Usage: ./generateTangled.sh inputConfiguration outputDirectory

inputConfiguration=$1
outputDirectory=$2

mkdir -p ${outputDirectory}

echo "" > /tmp/commands.txt

for steps in 500 1000 4000 8000 16000; do
    for i in $(seq -w 10); do
        output=${outputDirectory}/$(basename $inputConfiguration.$steps.$i)
        echo rofi-tangle --steps ${steps} $inputConfiguration $output >> /tmp/commands.txt
    done
done

echo "Running it in parallel:"
parallel < /tmp/commands.txt
