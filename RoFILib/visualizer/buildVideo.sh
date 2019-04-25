#!/bin/bash

INPATH=
OUTPATH=../data/animation/output.mp4
OSET=FALSE
FRAMERATE=24
FSET=FALSE
DELETE=FALSE
LOGO=TRUE

# parse input parameters

while [[ $# -gt 0 ]]
do 
key="$1"

case $key in
	-i|--input)
        if [ "$INPATH" != "" ]
          then 
            echo "There can be at most one -i or --input option."
            exit 1
        fi
	INPATH="$2"
	shift
	shift
	;;
	-o|--output)
        if [ "$OSET" == "TRUE" ]
          then 
            echo "There can be at most one -o or --outpath option."
            exit 1
        fi
	OUTPATH="$2"
        OSET=TRUE
	shift
	shift
	;;
	-f|--framerate)
        if [ "$FSET" == "TRUE" ]
          then 
 	    echo "There can be at most one -f or --framerate option."
 	    exit 1
	fi
	FRAMERATE="$2"
	FSET=TRUE
	shift
	shift
	;;
	-d|--delete)
	DELETE=TRUE
	shift
	;;
	-l|--nologo)
	LOGO=FALSE
	shift
	;;
	-h|--help)
	echo "  -h, --help         Prints help"
	echo "  -i, --input        Directory with pictures to be animated"
	echo "  -o, --output       Output file (path/videoName.mp4)"
	echo "  -f, --framerate    Number of pictures per second"
	echo "  -d, --delete       Delete pictures in the input directory"
	echo "  -l, --nologo       Video without RoFI logo"
	exit 0
	;;
	*)
	echo "Unknown argument $1"
	exit 1
	;;
esac
done

if [ "$INPATH" == "" ]
  then 
    echo "There must be exactly one -i or --input option"
    exit 1
fi


AUXILIARY1=`mktemp /tmp/XXXXXX.mp4`
AUXILIARY2=`mktemp /tmp/XXXXXX.mp4`

# create video from simple pictures
ffmpeg -y -framerate $FRAMERATE -i ${INPATH}/img%04d.png -pix_fmt yuv420p $AUXILIARY1

# repeat last frame for 1 second 
ffmpeg -y -i $AUXILIARY1 -filter_complex "[0]trim=0:1[hold];[0][hold]concat[extended];[extended][0]overlay" $AUXILIARY2

if [ "$LOGO" == "TRUE" ]
  then
    # add logo
    ffmpeg -i $AUXILIARY2 -i rofi-logo.png -filter_complex 'overlay=x=W-w-30:y=H-h-20' $OUTPATH
  else
    cp $AUXILIARY2 $OUTPATH
fi

# remove auxiliary video
rm $AUXILIARY1
rm $AUXILIARY2

[ "$DELETE" == "TRUE" ] && rm ${INPATH}/img*.png
