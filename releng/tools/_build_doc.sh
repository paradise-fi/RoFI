#!/usr/bin/env bash

DOC_DIR=${ROFI_ROOT}/doc

if !( command -v inotifywait >/dev/null 2>&1 ); then
    echo >&2 "ERROR: could not find 'inotifywait'. Aborting."
    exit 1
fi

trap 'kill $SERVERPID; exit' INT

rmake doc
cd ${ROFI_BUILD_DIR}/doc/web && python3 -m http.server 3333 &
SERVERPID=$!

if command -v xdg-open >/dev/null 2>&1 ; then
    xdg-open http://localhost:3333
else
    echo >&2 "ERROR: Could not find 'xdg-open'. Open: http://localhost:3333"
fi


while true; do
    inotifywait -qre close_write $DOC_DIR
    rmake doc
done
