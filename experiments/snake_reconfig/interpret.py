#!/usr/bin/env python3

import click
import json
from textwrap import indent
import os
import math

PROGRESS = {
    -1: "Timeout/Out of memory",
    0: "Failed on aerate",
    1: "Failed on tree to chain",
    2: "Failed on fixing parity",
    3: "Failed on fixing docks",
    4: "Failed on flattening circle",
    5: "Finished"
}

def moduleCountFromName(name):
    name = os.path.basename(name)
    if name.startswith("m"):
        return int(name.replace("_", "-").split("-")[0][1:])
    return int(name.split("-")[0])

@click.command()
@click.argument("source", type=click.Path(file_okay=True, dir_okay=False, exists=True))
def basestats(source):
    with open(source) as f:
        data = json.load(f)
    tasks = data["tasks"]

    sucs = {k: 0 for k in PROGRESS.keys()}
    for t in tasks:
        if t["result"] is not None and  "progress" in t["result"]:
            p = t["result"]["progress"]
            sucs[p] += 1
        else:
            sucs[-1] += 1
    sorted = [(k, v) for k, v in sucs.items()]
    sorted.sort(key=lambda x: x[0])
    for k, v in sorted:
        print(f"{PROGRESS[k]}: {v}")

def tableHeader(datasets, limits):
    count = len(datasets)
    limitCols = len(limits) + 1
    res = r"\begin{tabular}{@{}rr" + "|".join(count * [limitCols * "c"]) + r"@{}}" + "\n"
    res += r"\toprule" + "\n"

    res += "    Dataset & "
    for i, name in enumerate(datasets):
        endchar = "" if i == len(datasets) - 1 else "|"
        res += r"& \multicolumn{" + str(limitCols) + r"}{c" + endchar + r"}{" + name + r"}"
    res += r"\\" + "\n" + "   Module count & "
    for _ in datasets:
        l = [0] + list(limits) + [10000]
        for lower, upper in zip(l, l[1:]):
            if lower == 0:
                res += r"& $< " + str(upper) + "$"
            elif upper == 10000:
                res += r"& $\ge" + str(lower) + "$"
            else:
                res += f"& ${lower}--{upper}$ "

    res += r"\\ \toprule" + "\n"

    return res

def tableFooter():
    return r"\end{tabular}" + "\n"

def tableDataStats(dataset):
    total = len(dataset)
    finished = len([d for d in dataset if d["result"].get("progress", 0) == 5])
    failedOnArm = len([d for d in dataset if d["result"].get("progress", 0) == 1])
    timeData = [d["stats"]["cpuTime"] for d in dataset if d["result"].get("progress", 0) == 5]
    mean = f"{int(sum(timeData) / 1000000 / len(timeData))}s" if len(timeData) else "n/a"

    mu = sum(timeData) / len(timeData)
    deviation = sum([(x - mu)**2 for x in timeData]) / len(timeData)
    deviation = math.sqrt(deviation)
    deviation = deviation / 1000000

    res = r"\makecell{"
    res += f"{finished}/{total}\\\\"
    res += f"{failedOnArm}/{total}\\\\"
    res += mean
    res += "}"
    return res

def tableConditionRow(condName, limits, data, last):
    limits = [0] + list(limits) + [10000]
    files = data
    data = [json.load(open(x))["tasks"] for x in data]
    res = "    "
    res += r"\makecell[r]{" + condName.replace("\n", r"\\").replace(r"\n", r"\\") + "} "
    res += r"&\makecell[r]{S\\F\\T} "
    for i, dataset in enumerate(data):
        for lowerLimit, upperLimit in zip(limits, limits[1:]):
            dat = [d for d in dataset if
                d["result"] is not None and lowerLimit <= moduleCountFromName(d["result"]["input"]) < upperLimit]
            if len(dat) == 0:
                res += r" & n/a"
            else:
                res += r" & " + tableDataStats(dat)
    if last:
        res += r"\\ \bottomrule" + "\n"
    else:
        res += r"\\ \midrule" + "\n"

    return res

@click.command()
@click.option("--limit", type=int, multiple=True,
    help="Specify module count breakpoints")
@click.option("--name", type=str, multiple=True,
    help="Specify name of dataset")
@click.option("--data1", type=click.Path(file_okay=True, dir_okay=False, exists=True),
    multiple=True, help="Dataset source for condition 1")
@click.option("--data2", type=click.Path(file_okay=True, dir_okay=False, exists=True),
    multiple=True, help="Dataset source for condition 2")
@click.option("--data3", type=click.Path(file_okay=True, dir_okay=False, exists=True),
    multiple=True, help="Dataset source for condition 3")
@click.option("--cond", type=str, multiple=True,
    help="Condition name")
def table(limit, name, data1, data2, data3, cond):
    assert len(name) == len(data1) and \
        (len(data2) == 0 or len(name) == len(data2)) and \
        (len(data3) == 0 or len(name) == len(data3))
    dataname = name
    print(tableHeader(dataname, limit))
    print(tableConditionRow(cond[0], limit, data1, len(data2) == 0))
    if len(data2):
        print(tableConditionRow(cond[1], limit, data2, len(data3) == 0))
    if len(data3):
        print(tableConditionRow(cond[2], limit, data3, True))
    print(tableFooter())


@click.group()
def cli():
    pass

cli.add_command(basestats)
cli.add_command(table)


if __name__ == "__main__":
    cli()
