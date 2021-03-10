#!/usr/bin/env bash

if [ -d "$1" ]
then
    exit 0
fi

git clone https://github.com/Z3Prover/z3.git $1
cd $1
git checkout $2
mkdir -p _sysroot
SYSROOT=`realpath _sysroot`

export CXXFLAGS='-std=c++98'
python scripts/mk_make.py --prefix=$SYSROOT
cd build
make -j60 || exit 1
make install