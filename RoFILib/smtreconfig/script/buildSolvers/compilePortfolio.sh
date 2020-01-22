#!/usr/bin/env bash

DIR=portfolio2

mkdir -p $DIR

./compileZ3.sh $DIR/z3-upstream da2f5cc || exit 1
./compileZ3.sh $DIR/z3-4.8.7 30e7c22 || exit 1
./compileZ3.sh $DIR/z3-4.8.1 b301a59 || exit 1
./compileZ3.sh $DIR/z3-4.7.1 3b1b82b || exit 1
#./compileZ3.sh $DIR/z3-4.6.0 b0aaa4c || exit 1
./compileZ3.sh $DIR/z3-4.5.0 d57a2a6 || exit 1
./compileZ3.sh $DIR/z3-4.4.0 7f6ef0b || exit 1

./compileCVC4.sh $DIR/cvc4-upstream 3de5e8d || exit 1
./compileCVC4.sh $DIR/cvc4-1.7 84da9c0 || exit 1
./compileCVC4.sh $DIR/cvc4-1.5 05663e0 || exit 1
./compileCVC4.sh $DIR/cvc4-1.4 c0258d6 || exit 1
./compileCVC4.sh $DIR/cvc4-1.3 97d7c35 || exit 1

./compileSMTRAT.sh $DIR/smtrat-upstream 20c8b46 || exit 1
./compileSMTRAT.sh $DIR/smtrat-19.10 20c8b46 || exit 1
./compileSMTRAT.sh $DIR/smtrat-18.12 90e75db 2a32976 || exit 1
#./compileSMTRAT.sh $DIR/smtrat-2.2 8716bba || exit 1
#./compileSMTRAT.sh $DIR/smtrat-2.0 0bbd7ad || exit 1

./compileYices.sh $DIR/yices-upstream 1dcb71c || exit 1
./compileYices.sh $DIR/yices-2.6.1 ba13d37 || exit 1
./compileYices.sh $DIR/yices-2.5.0 25ecd47 || exit 1
./compileYices.sh $DIR/yices-2.4.0 1b50e9a 353eb26 || exit 1
