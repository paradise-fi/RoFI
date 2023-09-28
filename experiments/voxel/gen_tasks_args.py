#!/usr/bin/env python3

import click
import json
import subprocess
import sys

from collections import defaultdict
from typing import Optional, TextIO


class World:
    def __init__(self, file: str, format: str):
        self.file = file
        self.format = format

    def module_count(self) -> int:
        cmd = ["rofi-tool", "module-count", self.file, "--format", self.format]
        cmd_result = subprocess.run(cmd, capture_output=True)
        sys.stderr.buffer.write(cmd_result.stderr)

        if cmd_result.returncode != 0:
            print(
                f"Error while getting module count of '{self.file}' (format: {self.format})",
                file=sys.stderr,
            )
            exit(1)
        return int(cmd_result.stdout)

    def cmdsafe_file(self) -> str:
        return subprocess.list2cmdline([self.file])

    @staticmethod
    def get_snake_world(module_count: int) -> "World":
        assert module_count > 0
        return World(
            f"data/configurations/old/snake/m{module_count}_snake.rofi", format="old"
        )


class Task:
    def __init__(self, init: World, goal: World):
        self.init = init
        self.goal = goal

    def get_args(self):
        return {
            "init": self.init.file,
            "goal": self.goal.file,
            "init_fmt": self.init.format,
            "goal_fmt": self.goal.format,
        }


def print_task_args(tasks: list[Task], output: TextIO = sys.stdout):
    args_lines = [task.get_args() for task in tasks]
    json.dump(args_lines, indent=4, fp=output)
    print(file=output)


def generate_e2e_task_args(
    world_files: list[str], format: str, both_directions: bool
) -> list[Task]:
    by_module_count: dict[int, list[World]] = defaultdict(list)
    for world_file in world_files:
        world = World(world_file, format)
        by_module_count[world.module_count()].append(world)

    tasks: list[Task] = []
    for worlds in by_module_count.values():
        if len(worlds) == 1:
            print(
                f"World '{worlds[0]}' skipped - only one world with given module count",
                file=sys.stderr,
            )

        for i in range(0, len(worlds)):
            for j in range(i + 1, len(worlds)):
                tasks.append(Task(worlds[i], worlds[j]))
                if both_directions:
                    tasks.append(Task(worlds[j], worlds[i]))
    return tasks


def generate_snake_tasks(
    world_files: list[str], format: str, sizelimit: Optional[int]
) -> list[Task]:
    tasks: list[Task] = []
    by_module_counts: dict[int, int] = defaultdict(int)
    for world_file in world_files:
        world = World(world_file, format=format)
        module_count = world.module_count()

        if sizelimit is not None and by_module_counts[module_count] >= sizelimit:
            continue
        by_module_counts[module_count] += 1

        snake_world = World.get_snake_world(module_count)
        tasks.append(Task(world, snake_world))
    return tasks


@click.group()
def gen_task_args_cli():
    """Generate task set args."""
    pass


@gen_task_args_cli.command()
@click.argument(
    "world_files",
    type=click.Path(file_okay=True, dir_okay=False, exists=True),
    nargs=-1,
)
@click.option(
    "--format",
    "-f",
    type=click.Choice(["json", "voxel", "old"]),
    default="old",
    show_default=True,
    help="Format of world files",
)
@click.option(
    "--both-directions",
    "-b",
    is_flag=True,
    show_default=True,
    help="Generate both directions for every pair",
)
def e2e(world_files: list[str], format: str, both_directions: bool):
    """Generate task set args from WORLD_FILES from every world to every other world of the same module count."""
    tasks = generate_e2e_task_args(
        world_files, format=format, both_directions=both_directions
    )
    print_task_args(tasks)


@gen_task_args_cli.command()
@click.argument(
    "world_files",
    type=click.Path(file_okay=True, dir_okay=False, exists=True),
    nargs=-1,
)
@click.option(
    "--format",
    "-f",
    type=click.Choice(["json", "voxel", "old"]),
    default="old",
    show_default=True,
    help="Format of world files",
)
@click.option(
    "--sizelimit",
    type=click.IntRange(1),
    help="Include at most --sizelimit benchmarks of given module's count",
)
def snake(world_files: list[str], format: str, sizelimit: Optional[int]):
    """Generate task args set from WORLD_FILES to snake configuration."""
    tasks = generate_snake_tasks(world_files, format=format, sizelimit=sizelimit)
    print_task_args(tasks)


if __name__ == "__main__":
    gen_task_args_cli()
