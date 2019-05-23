#!/usr/bin/env bash

for serial in `st-info --probe | grep serial | cut -d ":" -f 2-`
do
    st-flash --serial $serial "$@" &
done

wait