#!/bin/bash

while getopts e:a:b:c:s:t:o:u: flag
do
    case "${flag}" in
        e) equalityType=${OPTARG};;
        a) startFormat=${OPTARG};;
        b) targetFormat=${OPTARG};;
        c) step=${OPTARG};;
        s) startConfig=${OPTARG};;
        t) targetConfig=${OPTARG};;
        o) outputDir=${OPTARG};;
        u) timeOut=${OPTARG};;
    esac
done

echo "Given arguments:";
echo "equalityType: $equalityType";
echo "startFormat: $startFormat";
echo "targetFormat: $targetFormat";
echo "step: $step";
echo "startConfig: $startConfig";
echo "targetConfig: $targetConfig";
echo "outputDir: $outputFile";
echo "timeOut: $timeOut";

rofi-isoreconfig --et $equalityType --sf $startFormat --tf $targetFormat --step=$step $startConfig $targetConfig $outputFile;
