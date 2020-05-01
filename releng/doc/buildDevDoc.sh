#!/usr/bin/env bash

# If you run this script locally, make sure you run it with environment variable
# SAFE_CHECKOUT=1, otherwise your git working index might get "messy"

set -e
BUILD_DIR=_build.doc
mkdir -p $BUILD_DIR
export BUILD_DIR=$(realpath $BUILD_DIR)
export ROFI_DIR=$(pwd)

BODY='
    NAME=$(echo %(refname) | cut -d/ -f3)
    TMP_DIR=$(mktemp -d)
    SRC_DIR=$TMP_DIR/source.$NAME/
    if [ -n "$SAFE_CHECKOUT" ]; then
        mkdir -p $SRC_DIR
        cd $SRC_DIR
        git init
        git remote add origin $ROFI_DIR
        git fetch --depth 1 origin %(objectname)
        git checkout %(objectname)
    else
        mkdir -p $SRC_DIR
        git --work-tree=$SRC_DIR checkout %(objectname) -- .
    fi

    if [ -e $SRC_DIR/doc ]; then
        echo "============= Building doc for branch $NAME ================="
        cd $SRC_DIR/doc
        pwd
        make
        mkdir -p $BUILD_DIR/branch/$NAME
        mv $SRC_DIR/doc/build/html/* $BUILD_DIR/branch/$NAME/
        echo "============= Done doc for branch $NAME ====================="
        echo
    else
        echo "Skipping branch $NAME: No documentation found"
    fi
    cd $BUILD_DIR
    rm -r $TMP_DIR
'

git for-each-ref --shell --format="$BODY" refs/heads/ | bash
