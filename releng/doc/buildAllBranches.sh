#!/usr/bin/env bash

set -e

BODY='
    NAME=$(echo %(refname) | cut -d/ -f3)
    TMP_DIR=$(mktemp -d)
    SRC_DIR=$TMP_DIR
    git archive %(objectname) | tar -x -C ${SRC_DIR}

    if [ -e $SRC_DIR/setup.sh ]; then
        echo ::group::Building doc for branch $NAME
        ( cd $TMP_DIR && \
          source setup.sh && \
          rcfg doc \
          && rmake doc )
        if [ "$?" ]; then
            mkdir -p $ROFI_BUILD_DIR/doc/web/branch/$NAME
            mv $TMP_DIR/build.Release/doc/web/* $ROFI_BUILD_DIR/doc/web/branch/$NAME/
        else
            echo "Build failed! Ignoring"
        fi
        echo ::endgroup::
    fi
    cd $ROFI_ROOT
    rm -r $TMP_DIR
'

git for-each-ref --shell --format="$BODY" refs/heads/ | bash
