#!/usr/bin/env python3

import subprocess
import sys
import click
import json
from typing import Any, Dict, List, TextIO


def safe_as_arg(arg: Any) -> str:
    return subprocess.list2cmdline([str(arg)])


# Json formatted dump with newline at the end
def dump_json(value: Any, file: TextIO, *, indent: int = 4):
    json.dump(value, indent=indent, fp=file)
    print(file=file)


@click.command()
@click.argument('task_args_file', type=click.File())
@click.option('--cmd', '-c', type=str, multiple=True, required=True,
              help='Formats of the task')
@click.option('--raw-args', is_flag=True,
              help='Pass the args as-is')
def gen_tasks_fmt(task_args_file: TextIO, cmd: List[str], raw_args: bool):
    """Generate task set from given format and args from TASK_ARGS_FILE (in json format).
    """
    task_args: List[Dict[str, Any]] = json.load(task_args_file)
    assert isinstance(task_args, list)
    assert all([isinstance(args, dict) for args in task_args])

    if not raw_args:
        task_args = [{k: safe_as_arg(args[k]) for k in args}
                     for args in task_args]

    tasks = [c.format_map(targs) for targs in task_args for c in cmd]
    dump_json(tasks, sys.stdout)


if __name__ == '__main__':
    gen_tasks_fmt()
