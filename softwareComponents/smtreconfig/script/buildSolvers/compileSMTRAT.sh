#!/usr/bin/env bash

if [ -d "$1" ]
then
    exit 0
fi

rm -rf ~/.cmake/packages/carl ~/.cmake/packages/smtrat

git clone https://github.com/smtrat/smtrat.git $1
cd $1
git checkout $2
mkdir -p _sysroot
SYSROOT=`realpath _sysroot`
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SYSROOT/lib/pkgconfig

Build CLN
git clone git://www.ginac.de/cln.git;
cd cln
./autogen.sh
./configure --prefix=$SYSROOT || exit 1
make -j60 || exit 1
make install
cd ..

# Build ginac
git clone git://www.ginac.de/ginac.git
cd ginac
autoreconf -i
./configure \
    --prefix=$SYSROOT || exit 1
make -j60 || exit 1
make install
cd ..

# Build carl
git clone https://github.com/smtrat/carl.git
cd carl
git checkout $3
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$SYSROOT USE_GINAC=OFF .. || exit 1
make -j60 || exit 1
make install
cd ../..
cp -r carl/cmake $SYSROOT/

# # Hack
# sed -i 's~find_package(carl ${CARL_REQUIRED_VERSION})~find_package(carl ${CARL_REQUIRED_VERSION} PATHS '$SYSROOT' NO_DEFAULT_PATH)~g' resources/resources.cmake
# # EndHack

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$SYSROOT ..
make -j60 || exit 1
make install
