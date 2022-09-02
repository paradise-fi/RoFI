#!/usr/bin/env python3

import json
import os
import click

@click.command()
@click.argument("directory", type=click.Path(file_okay=False, dir_okay=True, exists=True), nargs=-1)
def run(directory):
    res = []
    for d in directory:
        for f in os.listdir(d):
            for collision in ["none", "naive", "online"]:
                for straigt in ["no", "coll", "yes"]:
                    res.append(f"rofi-freconfig --log /artefact/results.json --collisions={collision} --straighten={straigt} {os.path.join(d, f)}")

    print(json.dumps(res, indent=4))


if __name__ == "__main__":
    run()
