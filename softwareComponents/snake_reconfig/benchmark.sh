set -x
LOG=log.csv
touch $LOG
FILES=../data/snakeBench/*
for f in $FILES
do
    ./snake_reconfig/snakeReconfig $f $LOG "${f}.out"
done
