#!/usr/bin/env python3

import click
import itertools
import json
import subprocess
import sys

from typing import Any, Iterable, TextIO, TypeVar, Union


T = TypeVar("T")
OneOrMore = Union[list[T], T]


def safe_as_arg(arg: Any) -> str:
    return subprocess.list2cmdline([str(arg)])


# Json formatted dump with newline at the end
def dump_json(value: Any, file: TextIO, *, indent: int = 4):
    json.dump(value, fp=file, indent=indent)
    print(file=file)


def all_combinations(matrix: dict[str, list[Any]]) -> Iterable[dict[str, Any]]:
    pair_matrix = [[(name, value) for value in matrix[name]] for name in matrix]
    return [dict(pairs) for pairs in itertools.product(*pair_matrix)]


@click.command(
    help="Generate task set from given format and args from TASK_ARGS_FILE (in json format) and cmd and matrix args from CMD_ARGS_FILE"
)
@click.argument("task_args_file", type=click.File())
@click.argument("cmd_args_file", type=click.File())
@click.option("--raw-args", is_flag=True, help="Pass the args as-is")
def gen_tasks_matrix(task_args_file: TextIO, cmd_args_file: TextIO, raw_args: bool):
    task_args: list[dict[str, Any]] = json.load(task_args_file)
    assert isinstance(task_args, list)
    assert all([isinstance(args, dict) for args in task_args])

    cmd_args: dict[str, Any] = json.load(cmd_args_file)
    assert isinstance(cmd_args, dict)
    cmd: str = cmd_args["cmd"]
    assert isinstance(cmd, str)

    matrix_args_list: OneOrMore[dict[str, OneOrMore[Any]]] = cmd_args["args"]
    if not isinstance(matrix_args_list, list):
        matrix_args_list = [matrix_args_list]
    for matrix_args in matrix_args_list:
        assert isinstance(matrix_args, dict)
        for key in matrix_args:
            if not isinstance(matrix_args[key], list):
                matrix_args[key] = [matrix_args[key]]

    if not raw_args:
        task_args = [
            {name: safe_as_arg(args[name]) for name in args} for args in task_args
        ]
        matrix_args_list = [
            {
                key: [safe_as_arg(value) for value in matrix_args[key]]
                for key in matrix_args
            }
            for matrix_args in matrix_args_list
        ]

    tasks = [
        cmd.format_map(margs | targs)
        for targs in task_args
        for matrix_args in matrix_args_list
        for margs in all_combinations(matrix_args)
    ]
    dump_json(tasks, sys.stdout)


if __name__ == "__main__":
    gen_tasks_matrix()
