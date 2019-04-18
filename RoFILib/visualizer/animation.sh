#!/bin/bash

PICTURESPATH="../data/res/tmp"
OUTPATH=../data/animation/output.mp4
OUTPATHSET=FALSE

# command line arguments for ./rofi-vis
VIS="-s -p $PICTURESPATH -a"

# command line arguments for videoCreator.sh
VID="-i $PICTURESPATH -d"

# parse input parameters

while [[ $# -gt 0 ]]
do
key="$1"

case $key in
     -h|--help)
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
     exit 0
     ;;
     -i|--input)
     VIS="$VIS -i $2"
     shift
     shift
     ;;
     -o|--output)
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
     VIS="$VIS -f $2"
     VID="$VID -f $2"
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
     *)
     echo "Unknown argument $1"
     exit 1
     ;;
esac
done

echo "VIS: $VIS"


[ "$OUTPATHSET" == "FALSE" ] && VID="$VID -o $OUTPATH"

echo "VID: $VID"

[ ! -d $PICTURESPATH ] && mkdir $PICTURESPATH
cd ../build && ./rofi-vis $VIS && cd ../visualizer && ./videoCreator.sh $VID && rmdir $PICTURESPATH



