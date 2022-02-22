#!/usr/bin/env python3

import sys
import os
from common import *

word = int(sys.argv[1])
words = sys.argv[3:]

candidates = ["all"] + availableSuites() + ["-" + x for x in configuredSuites()]
if "all" not in words:
    if word >= 1 and word - 1 < len(words):
        if words[word-1] in candidates:
            print(words[-1])
        candidates = [ s for s in candidates if s.startswith(words[word - 1]) ]
    candidates = set(candidates).difference(words)
    if len(candidates) > 0:
        print("\n".join(candidates))
if word == len(words) and words[-1] == "all":
    print("all")
