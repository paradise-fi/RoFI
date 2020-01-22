#!/usr/bin/env bash

if [ -d "$1" ]
then
    exit 0
fi

git clone https://github.com/CVC4/CVC4.git $1
cd $1
git checkout $2
mkdir -p _sysroot
SYSROOT=`realpath _sysroot`

pip3 install toml

if [ -f "autogen.sh" ]
then
    ./autogen.sh || exit 1
fi

sed -i "s/curl/culr -L/g" contrib/get-antlr-3.4
./contrib/get-antlr-3.4 || exit 1

if [ -f "autogen.sh" ]
then
    sed -i 's/boost_cv_lib_version=`cat conftest.i`/boost_cv_lib_version="1_67_0_1"/g' configure
    ./configure --prefix=$SYSROOT --with-antlr-dir=`realpath antlr-3.4` || exit 1
    exit 0
else
    ./configure.sh --python3 --prefix=$SYSROOT || exit 1
    cd build
fi
make -j60 || exit 1
make install
