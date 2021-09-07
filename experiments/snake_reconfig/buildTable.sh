#!/usr/bin/env bash

./interpret.py table \
    --limit 10 \
    --cond "Conservative\nlimits" --cond "Generous\nlimits" --cond "Semi-\nconservative\nlimits" \
    --name "Hand crafted" \
        --data1 results/rofibots.json \
        --data2 results/rofibots-I1.json \
        --data3 results/rofibots-I2.json \
    --name "Random" \
        --data1 results/mb.json \
        --data2 results/mb-I1.json \
        --data3 results/mb-I2.json \
    --name "Tangled" \
        --data1 results/tangled1k.json \
        --data2 results/tangled1k-I1.json \
        --data3 results/tangled1k-I2.json
