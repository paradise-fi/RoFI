#!/usr/bin/env python3

import sys
import os
import shutil
import json
import argparse
from typing import TextIO, List


def dir_path(path: str) -> str:
    if not os.path.isdir(path):
        raise NotADirectoryError(path)
    return path


def get_rust_executables(json_input: TextIO) -> List[str]:
    executables: list[str] = []

    for line in json_input:
        artifact = json.loads(line)
        if artifact['reason'] != 'compiler-artifact':
            continue
        executable = artifact['executable']
        if executable is None:
            continue
        if type(executable) is not str:
            raise TypeError("executable in json has to be a string")
        executables.append(executable)

    return executables


def get_filename(filepath: str, prefix: str = '') -> str:
    assert os.path.isfile(filepath)
    return prefix + os.path.basename(filepath)


def copy_executables(source: TextIO, dest_dir: str, prefix: str = ''):
    executables = get_rust_executables(source)
    if len(executables) == 0:
        raise RuntimeError("No executables")

    for executable in executables:
        filename = get_filename(executable, prefix)
        shutil.copy2(executable, os.path.join(dest_dir, filename))


def create_script(source: TextIO, script_path: str):
    executables = get_rust_executables(source)
    if len(executables) == 0:
        raise RuntimeError("No executables")

    with open(script_path, 'w', encoding='UTF-8') as script_file:
        script_file.write('#!/usr/bin/env bash\n\n')
        script_file.write('\n'.join(executables) + '\n')
        old_mode = os.stat(script_file.fileno()).st_mode
        os.chmod(script_file.fileno(), old_mode | 0o111)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Tool for working with rust executable files')
    subparsers = parser.add_subparsers(required=True)

    parser_copy = subparsers.add_parser('copy', help='extract and copy executables to destination directory',
                                        description='Extract and copy executables to destination directory.')
    parser_copy.add_argument('dest_dir', type=dir_path,
                             help='destination directory for the executables')
    parser_copy.add_argument('--prefix', action='store', default='',
                             help='prefix to add to the executables')
    parser_copy.set_defaults(
        func=lambda args: copy_executables(sys.stdin, args.dest_dir, args.prefix))

    parser_script = subparsers.add_parser('script', help='create a script that calls all executables',
                                          description='Create a script that calls all executables.')
    parser_script.add_argument('script_path',
                               help='path where to create the script')
    parser_script.set_defaults(
        func=lambda args: create_script(sys.stdin, args.script_path))

    args = parser.parse_args()
    args.func(args)
