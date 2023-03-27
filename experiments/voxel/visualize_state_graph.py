#!/usr/bin/env python3

from dataclasses import dataclass
import click
import itertools
import json
from prettytable import PrettyTable
from typing import Any, Dict, Iterable, List, Literal, Optional, TextIO, Tuple

from utils import parse_json

TableType = Literal["both", "avg", "sum"]


@dataclass(kw_only=True)
class SuccessorsCall:
    modules: int
    successful_cuts: int
    failed_cuts: int
    duplicate_moves: int
    collided_moves: int
    new_unique_states: int

    @property
    def all_moves(self) -> int:
        return self.collided_moves + self.duplicate_moves + self.new_unique_states

    @property
    def all_cuts(self) -> int:
        return self.successful_cuts + self.failed_cuts


@dataclass(kw_only=True)
class Counters:
    sum: SuccessorsCall
    count: int


def get_table_head(label_head: str):
    return [
        label_head,
        "Succ Calls",
        "M",
        "Cuts - Success",
        "Cuts - Fail",
        "New Unique Worlds",
        "Moves - Duplicate",
        "Moves - Collision",
    ]


def get_table_sum_row(label: str, calls: Counters):
    assert calls.count != 0
    return [
        label,
        calls.count,
        calls.sum.modules // calls.count
        if calls.sum.modules % calls.count == 0
        else calls.sum.modules / calls.count,
        calls.sum.successful_cuts,
        calls.sum.failed_cuts,
        calls.sum.new_unique_states,
        calls.sum.duplicate_moves,
        calls.sum.collided_moves,
    ]


def get_table_avg_row(label: str, calls: Counters, *, div_by: Optional[int] = None):
    assert div_by != 0
    assert calls.count != 0
    div_by = calls.count if div_by is None else div_by
    return [
        label,
        calls.count,
        calls.sum.modules // calls.count
        if calls.sum.modules % calls.count == 0
        else calls.sum.modules / calls.count,
        calls.sum.successful_cuts / div_by,
        calls.sum.failed_cuts / div_by,
        calls.sum.new_unique_states / div_by,
        calls.sum.duplicate_moves / div_by,
        calls.sum.collided_moves / div_by,
    ]


def get_table_row(label: str, calls: Counters, *, type: TableType, show_percent: bool):
    successful_cuts = CounterCell.from_sum(
        sum=calls.sum.successful_cuts, len=calls.count, total=calls.sum.all_cuts
    )
    failed_cuts = CounterCell.from_sum(
        sum=calls.sum.failed_cuts, len=calls.count, total=calls.sum.all_cuts
    )
    new_unique_states = CounterCell.from_sum(
        sum=calls.sum.new_unique_states, len=calls.count, total=calls.sum.all_moves
    )
    duplicate_moves = CounterCell.from_sum(
        sum=calls.sum.duplicate_moves, len=calls.count, total=calls.sum.all_moves
    )
    collided_moves = CounterCell.from_sum(
        sum=calls.sum.collided_moves, len=calls.count, total=calls.sum.all_moves
    )
    return [
        label,
        calls.count,
        calls.sum.modules // calls.count
        if calls.sum.modules % calls.count == 0
        else calls.sum.modules / calls.count,
        successful_cuts.to_string(type=type, show_percent=show_percent),
        failed_cuts.to_string(type=type, show_percent=show_percent),
        new_unique_states.to_string(type=type, show_percent=show_percent),
        duplicate_moves.to_string(type=type, show_percent=show_percent),
        collided_moves.to_string(type=type, show_percent=show_percent),
    ]


def get_table(
    rows: Iterable[Tuple[str, Counters]],
    *,
    head: str,
    type: TableType,
    show_percent: bool,
) -> PrettyTable:
    table = PrettyTable(get_table_head(head), align="r", float_format=".2")

    for name, row in rows:
        row = get_table_row(label=name, calls=row, type=type, show_percent=show_percent)
        table.add_row(row)  # type: ignore
    return table


@dataclass(kw_only=True)
class CountersResult:
    total: Counters
    bfs_layers: Optional[List[Counters]]

    def get_bfs_table(
        self, *, type: TableType, show_percent: bool = True
    ) -> PrettyTable:
        assert self.bfs_layers is not None

        rows = itertools.chain(
            [("Total", self.total)],
            iter((f"Layer {i}", calls) for i, calls in enumerate(self.bfs_layers, 1)),
        )
        return get_table(rows, head="Layer", type=type, show_percent=show_percent)


class CounterCell:
    def __init__(
        self,
        *,
        counter_sum: int,
        counter_avg: Optional[float],
        percentage: Optional[float],
    ):
        self.counter_sum = counter_sum
        self.counter_avg = counter_avg
        self.percentage = percentage

    @staticmethod
    def from_sum(*, sum: int, len: int, total: int) -> "CounterCell":
        return CounterCell(
            counter_sum=sum,
            counter_avg=sum / len if len != 0 else None,
            percentage=100 * sum / total if total != 0 else None,
        )

    def to_string(self, *, type: TableType, show_percent: bool = True) -> str:
        result: List[str] = []
        if type == "both" or type == "sum":
            result.append(f"{self.counter_sum}")
        if type == "both" or type == "avg":
            result.append(
                f"{self.counter_avg:.2f}" if self.counter_avg is not None else "-"
            )
        if show_percent:
            result.append(
                f"{self.percentage:.0f} %" if self.percentage is not None else "-"
            )

        return " | ".join(result)


@click.group()
def visualize_state_graph():
    pass


@visualize_state_graph.command()
@click.argument("counters_data_file", type=click.File())
@click.option(
    "--type",
    "-t",
    "type_",
    type=click.Choice(["both", "avg", "sum"]),
    default="both",
    show_default=True,
)
@click.option(
    "--no-percent",
    "--np",
    "percent",
    is_flag=True,
    default=True,
    help="Don't show percentages",
)
def bfs_layers(
    counters_data_file: TextIO, type_: Literal["both", "avg", "sum"], percent: bool
):
    counters_data = json.load(counters_data_file)
    counters = parse_json(CountersResult, counters_data)
    assert isinstance(counters, CountersResult)

    table = counters.get_bfs_table(type=type_, show_percent=percent)
    print(table)


@visualize_state_graph.command()
@click.argument("counters_data_file", type=click.File(), nargs=-1)
@click.option(
    "--type",
    "-t",
    "type_",
    type=click.Choice(["both", "avg", "sum"]),
    default="both",
    show_default=True,
    help="Which data to show",
)
@click.option(
    "--no-percent",
    "--np",
    "percent",
    is_flag=True,
    default=True,
    help="Don't show percentages",
)
def total(
    counters_data_file: List[TextIO],
    type_: Literal["both", "avg", "sum"],
    percent: bool,
):
    all_counters: List[Tuple[str, Counters]] = []
    for file in counters_data_file:
        counter_data = json.load(file)
        counter = parse_json(CountersResult, counter_data)
        assert isinstance(counter, CountersResult)

        all_counters.append((file.name, counter.total))

    table = get_table(all_counters, head="File", type=type_, show_percent=percent)
    print(table)


@visualize_state_graph.command()
@click.argument("results_file", type=click.File())
@click.option(
    "--type",
    "-t",
    "type_",
    type=click.Choice(["both", "avg", "sum"]),
    default="both",
    show_default=True,
    help="Which data to show",
)
@click.option(
    "--no-percent",
    "--np",
    "percent",
    is_flag=True,
    default=True,
    help="Don't show percentages",
)
def total_json_summary_results(
    results_file: TextIO, type_: Literal["both", "avg", "sum"], percent: bool
):
    all_counters: List[Tuple[str, Counters]] = []

    results: Dict[str, Any] = json.load(results_file)
    assert isinstance(results, dict)
    assert "tasks" in results
    tasks: List[Any] = results["tasks"]
    assert isinstance(tasks, list)
    for i, task in enumerate(tasks, 1):
        assert isinstance(task, dict)
        assert "result" in task
        task_results: Optional[Dict[str, Any]] = task["result"]
        if task_results is None:
            continue

        counter = parse_json(CountersResult, task_results)
        assert isinstance(counter, CountersResult)

        all_counters.append((f"Task {i}", counter.total))

    table = get_table(all_counters, head="File", type=type_, show_percent=percent)
    print(table)


if __name__ == "__main__":
    visualize_state_graph()
