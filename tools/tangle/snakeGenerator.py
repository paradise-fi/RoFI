#!/usr/bin/env python3

import click

@click.command()
@click.option("-l", "--length", type=int)
def run(length):
    print("C")
    for i in range(length):
        print(f"M {i} 0 0 0")
    print("")
    for i in range(length - 1):
        print(f"E {i} 1 2 0 2 0 {i + 1}")

if __name__ == "__main__":
    run()