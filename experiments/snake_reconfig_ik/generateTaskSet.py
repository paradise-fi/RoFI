#!/usr/bin/env python3

import json
import os
import click

def moduleCount(filename) -> int:
    with open(filename) as f:
        return sum(1 for line in f.readlines() if line.startswith("M"))

@click.command()
@click.argument("directory", type=click.Path(file_okay=False, dir_okay=True, exists=True), nargs=-1)
@click.option("--sizelimit", "-s", type=int, default=None,
    help="Include at most <sizelimit> benchmarks of given module count")
def run(directory, sizelimit):
    res = []
    for d in directory:
        sizeCounts = {}
        for f in os.listdir(d):
            benchmark = os.path.join(d, f)
            size = moduleCount(benchmark)

            if sizelimit is not None and sizeCounts.get(size, 0) >= sizelimit:
                continue
            sizeCounts[size] = sizeCounts.get(size, 0) + 1

            for collision in ["none", "naive", "online"]:
                for straigt in ["no", "coll", "yes"]:
                    res.append(f"rofi-freconfig --log /artefact/results.json --collisions={collision} --straighten={straigt} {benchmark}")

    print(json.dumps(res, indent=4))


if __name__ == "__main__":
    run()
