#!/usr/bin/env python3

import click
import itertools
import json
import subprocess
import sys

from dataclasses import dataclass
from typing import Any, Iterable, TextIO, TypeVar, Union
from utils import OneOrMore, parse_json


@dataclass(kw_only=True)
class ArgsFile:
    cmd: str
    args: OneOrMore[dict[str, OneOrMore[Any]]]


def safe_as_arg(arg: Any) -> str:
    return subprocess.list2cmdline([str(arg)])


# Json formatted dump with newline at the end
def dump_json(value: Any, file: TextIO, *, indent: int = 4):
    json.dump(value, fp=file, indent=indent)
    print(file=file)


def all_combinations(matrix: dict[str, list[Any]]) -> Iterable[dict[str, Any]]:
    pair_matrix = [[(name, value) for value in matrix[name]] for name in matrix]
    return [dict(pairs) for pairs in itertools.product(*pair_matrix)]


def all_arg_combinations(
    matrix_args_list: OneOrMore[dict[str, OneOrMore[Any]]], *, escape_args: bool
) -> list[dict[str, Any]]:
    if not isinstance(matrix_args_list, list):
        matrix_args_list = [matrix_args_list]
    for matrix_args in matrix_args_list:
        assert isinstance(matrix_args, dict)
        for key in matrix_args:
            if not isinstance(matrix_args[key], list):
                matrix_args[key] = [matrix_args[key]]

    if escape_args:
        matrix_args_list = [
            {
                key: [safe_as_arg(value) for value in matrix_args[key]]
                for key in matrix_args
            }
            for matrix_args in matrix_args_list
        ]

    return [
        margs
        for matrix_args in matrix_args_list
        for margs in all_combinations(matrix_args)
    ]


@click.command(
    help="Generate task set from given format and args from TASK_ARGS_FILE (in json format) and cmd and matrix args from CMD_ARGS_FILE"
)
@click.argument("task_args_file", type=click.File())
@click.argument("cmd_args_file", type=click.File())
@click.option(
    "--escape-args",
    is_flag=True,
    help="Pass each arg as one argument to underlying command",
)
def gen_tasks_matrix(task_args_file: TextIO, cmd_args_file: TextIO, escape_args: bool):
    task_args = parse_json(list[dict[str, Any]], json.load(task_args_file))
    cmd_args = parse_json(ArgsFile, json.load(cmd_args_file))

    if escape_args:
        task_args = [
            {name: safe_as_arg(args[name]) for name in args} for args in task_args
        ]

    tasks = [
        cmd_args.cmd.format_map(margs | targs)
        for targs in task_args
        for margs in all_arg_combinations(cmd_args.args, escape_args=escape_args)
    ]
    dump_json(tasks, sys.stdout)


if __name__ == "__main__":
    gen_tasks_matrix()
