#!/usr/bin/env bash

if [ -d "$1" ]
then
    exit 0
fi

git clone https://github.com/SRI-CSL/yices2.git $1
cd $1
git checkout $2
mkdir -p _sysroot
SYSROOT=`realpath _sysroot`

# Compile libpoly
git clone https://github.com/SRI-CSL/libpoly.git
cd libpoly
git checkout $3
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$SYSROOT || exit 1
make -j60 || exit 1
make install
cd ../..

# Compile cudd
git clone https://github.com/ivmai/cudd.git
cd cudd
sed -i "s/am__api_version='1.14'/am__api_version='1.16'/g" configure
./configure --enable-shared --enable-obj --enable-silent-rules --prefix $SYSROOT
make -j60 || exit 1
make install
cd ..

# Compile yieces
autoconf
./configure --enable-mcsat \
    LDFLAGS="-L$SYSROOT/lib" \
    CPPFLAGS="-I$SYSROOT/include" \
    --prefix=$SYSROOT || exit 1
make -j60 || exit 1
make install
