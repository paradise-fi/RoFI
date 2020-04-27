#!/usr/bin/env bash

DOC_DIR=.

trap 'kill $SERVERPID; exit' INT

make
python3 -m http.server -d build/html 3333 &
SERVERPID=$!

xdg-open http://localhost:3333

while true; do
    inotifywait -qre close_write $DOC_DIR
    make
done;