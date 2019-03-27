#!/bin/bash

input=".videoParams.txt"
in=$(sed -n 1p ${input})
out=../data/animation/output.mp4
framerate=$(sed -n 2p ${input})
[ "$#" -eq 3 ] && out=$1 && in=$2 && framerate=$3
[ "$#" -eq 2 ] && out=$1 && in=$2
[ "$#" -eq 1 ] && out=$1
ffmpeg -framerate $framerate -i ${in}/img%04d.png -pix_fmt yuv420p $out
