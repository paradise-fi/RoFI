#!/usr/bin/env python3

import json
import sys

data = json.load(sys.stdin)

CHUNK_SIZE = 7000
for i, chunk in enumerate([data[i:i + CHUNK_SIZE] for i in range(0, len(data), CHUNK_SIZE)]):
    with open(f"experiments_{i+1}.json", "w") as f:
        json.dump(list(chunk), f)
