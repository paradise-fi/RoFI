set -x
touch log.txt
FILES=../data/snakeBench/*
for f in $FILES
do
    ./snake_reconfig/snakeReconfig $f log.txt "${f}.out"
done