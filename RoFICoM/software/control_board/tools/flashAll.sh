#!/usr/bin/env bash

FLASHED=0
for serial in `st-info --probe | grep " serial" | cut -d ":" -f 2-`
do
    st-flash --serial $serial "$@"
    FLASHED=1
done

if [ $FLASHED -eq 0 ]
then
    echo "Flashing error - no device connected?"
    exit 1
fi

wait
st-info --probe