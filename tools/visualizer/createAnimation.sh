#!/bin/bash

PICTURESPATH=`mktemp -d /tmp/XXXXXX`
OUTPATHSET=FALSE
INPUTSET=FALSE
FRAMERATESET=FALSE

# command line arguments for rofi-vis
VIS="-s -p $PICTURESPATH -a"

# command line arguments for rofi-vis-build-video
VID="-i $PICTURESPATH -d"

# parse input parameters

while [[ $# -gt 0 ]]
do
key="$1"

case $key in
     -h|--help)
     echo "RoFI video: Script for creating animation from configuration file"
     echo "Usage: "
     echo "  rofi-vis-video [OPTIONS]"
     echo ""
     echo "  -h, --help            Prints help"
     echo "  -i, --input arg       Input config file"
     echo "  -o, --output arg      Output file (path/videoName.mp4)"
     echo "  -c, --camera arg      Camera settings file"
     echo "  -f, --framerate arg   Number of pictures per second"
     echo "  -v, --velocity arg    Maximal angular velocity in 1°/s"
     echo "  -g, --angle arg       Maximal angle diff in ° per picture"
     echo "  -t, --recTime arg     Time in seconds for reconnection"
     echo "  -e, --recPics arg     Number of pictures for reconnection"
     echo "  -r, --resolution arg  Resolution of the animation"
     echo "  -m, --magnify arg     Magnification of the resolution"
     echo "  -l, --nologo          Video without RoFI logo"
     exit 0
     ;;
     -i|--input)
     if [ "$INPUTSET" == "TRUE" ]
	then 
	  echo "There can be at most one -i or --input option."
   	  exit 1
     fi
     VIS="$VIS -i $2"
     INPUTSET=TRUE
     shift
     shift
     ;;
     -o|--output)
     if [ "$OUTPATHSET" == "TRUE" ]
	then 
	  echo "There can be at most one -o or --output option."
   	  exit 1
     fi
     VID="$VID -o $2"
     OUTPATHSET=TRUE
     shift
     shift
     ;;
     -c|--camera)
     VIS="$VIS -c $2"
     shift
     shift
     ;;
     -f|--framerate)
     if [ "$FRAMERATESET" == "TRUE" ]
	then 
	  echo "There can be at most one -f or --framerate option."
   	  exit 1
     fi
     VIS="$VIS -f $2"
     VID="$VID -f $2"
     FRAMERATESET=TRUE
     shift
     shift
     ;;
     -v|--velocity)
     VIS="$VIS -v $2"
     shift
     shift
     ;;
     -g|--angle)
     VIS="$VIS -g $2"
     shift
     shift
     ;;
     -t|--recTime)
     VIS="$VIS -t $2"
     shift
     shift
     ;;
     -e|--recPics)
     VIS="$VIS -e $2"
     shift
     shift
     ;;
     -r|--resolution)
     VIS="$VIS -r $2"
     shift
     shift
     ;;
     -m|--magnify)
     VIS="$VIS -m $2"
     shift
     shift
     ;;
     -l|--nologo)
     VID="$VID -l"
     shift 
     ;;
     *)
     echo "Unknown argument $1"
     exit 1
     ;;
esac
done


if [ "$OUTPATHSET" == FALSE ] 
    then 
      echo "There must be exactly one -o or --output option"
      exit 1
fi

echo "VIS: $VIS"
echo "VID: $VID"

[ ! -d $PICTURESPATH ] && mkdir $PICTURESPATH
rofi-vis $VIS && rofi-vis-build-video $VID

# clean up temporary images
rm -r $PICTURESPATH




