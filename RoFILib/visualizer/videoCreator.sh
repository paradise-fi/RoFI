#!/bin/bash

input=".videoParams.txt"
INPATH=$(sed -n 1p ${input})
OUTPATH=../data/animation/output.mp4
FRAMERATE=$(sed -n 2p ${input})
DELETE=FALSE

while [[ $# -gt 0 ]]
do 
key="$1"

case $key in
	-i|--input)
	INPATH="$2"
	shift
	shift
	;;
	-o|--output)
	OUTPATH="$2"
	shift
	shift
	;;
	-f|--framerate)
	FRAMERATE="$2"
	shift
	shift
	;;
	-d|--delete)
	DELETE=TRUE
	shift
	;;
	-h|--help)
	echo "  -h, --help         Prints help"
	echo "  -i, --input        Directory with pictures to be animated"
	echo "  -o, --output       Output file (path/videoName.mp4)"
	echo "  -f, --framerate    Number of pictures per second"
	echo "  -d, --delete       Delete pictures in the input directory"
	exit 0
	;;
	*)
	echo "Unknown argument $1"
	exit 1
	;;
esac
done

ffmpeg -framerate $FRAMERATE -i ${INPATH}/img%04d.png -pix_fmt yuv420p $OUTPATH

[ "$DELETE" == TRUE ] && rm ${INPATH}/img*.png
