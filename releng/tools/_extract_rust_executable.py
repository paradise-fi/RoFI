#!/usr/bin/env python3

import sys
import json


def get_rust_executable(json_input):
    executable = None

    for line in json_input:
        artifact = json.loads(line)
        if artifact['reason'] != 'compiler-artifact':
            continue
        if artifact['executable'] is None:
            continue

        if executable is not None:
            raise RuntimeError("Multiple executables")
        executable = artifact['executable']

    if executable is None:
        raise RuntimeError("No executables")

    return executable


if __name__ == '__main__':
    executable = get_rust_executable(sys.stdin)
    print(executable)
