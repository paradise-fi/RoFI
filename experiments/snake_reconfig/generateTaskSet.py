#!/usr/bin/env python3

import json
import os
import click

@click.command()
@click.argument("directory", type=click.Path(file_okay=False, dir_okay=True, exists=True))
def run(directory):
    res = []
    for f in os.listdir(directory):
        res.append(f"snakeReconfig --log /artefact/results.json {os.path.join(directory, f)}")

    print(json.dumps(res, indent=4))


if __name__ == "__main__":
    run()