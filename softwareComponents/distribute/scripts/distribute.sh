#!/bin/bash

function help() {
    cat <<EOF
  -h, --help           Print help
  -i, --init arg       Initial config file
  -g, --goal arg       Goal config file
  -d, --directory arg  Directory for module's configs
  -f, --hostfile arg   Host's file for mpiexec
  -a, --alg arg        Algorithm for reconfiguration: 
                       full, partial
  -e, --eval arg       Evaluation function for full 
                       A* algorithm: trivial, dMatrix, 
                       dCenter, dJoint, dAction
EOF
}

while [[ $# -gt 0 ]]; do
	PARAM="$1"

	case $PARAM in
	-h|--help)
        help
		exit 0
		;;
    -i|--init)
        if [ ! -z ${INIT+x} ]; then
            echo "Error: Multiple -i/--init options present." >&2
            exit 1
        fi
        INIT="$2"
        shift 2
        ;;
    -g|--goal)
        if [ ! -z ${GOAL+x} ]; then
            echo "Error: Multiple -g/--goal options present." >&2
            exit 1
        fi
        GOAL="$2"
        shift 2
        ;;
    -d|--directory)
        if [ ! -z ${DIR+x} ]; then
            echo "Error: Multiple -d/--directory options present." >&2
            exit 1
        fi
        DIR="$2"
        shift 2
        ;;
    -f|--hosftile)
        if [ ! -z ${HOSTS+x} ]; then
            echo "Error: Multiple -f/--hostfile options present." >&2
            exit 1
        fi
        HOSTS="$2"
        shift 2
        ;;
    -a|--alg)
        if [ ! -z ${ALG+x} ]; then
            echo "Error: Multiple -a/--alg options present." >&2
            exit 1
        fi
        ALG="$2"
        shift 2
        ;;
    -e|--eval)
        if [ ! -z ${EVAL+x} ]; then
            echo "Error: Multiple -e/--eval options present." >&2
            exit 1
        fi
        EVAL="$2"
        shift 2
        ;;
    *)
        echo "Error: Unknown parameter $1." >&2
        help
        exit 1
        ;;
    esac
done

DICTIONARY="$DIR/dictionary"
COUNT="$DIR/count"
OUT_MPI="run.out"

mkdir $DIR

../../build/rofi-distribute-preprocessing -i $INIT -g $GOAL -d $DIR &&
  ROFI_COUNT=$(head -n 1 $COUNT) &&
  mpiexec --hostfile $HOSTS -np $ROFI_COUNT ../../build/distribute/rofi-distribute -d $DIR -a $ALG -e $EVAL >> $OUT_MPI &&
  ../../build/rofi-distribute-postprocessing -i $OUT_MPI -t $DICTIONARY
