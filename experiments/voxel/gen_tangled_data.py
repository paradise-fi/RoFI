#!/usr/bin/env python3

from concurrent import futures
import click
import os
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from typing import List


def run_rofi_tangle(source: str, output: str, steps: int, log: str):
    print(log, file=sys.stderr)
    subprocess.check_call(['rofi-tangle', f'--steps={steps}', source, output])


@click.command()
@click.argument('world_files', type=click.Path(file_okay=True, dir_okay=False, exists=True), nargs=-1)
@click.option('--output_dir', '-o', type=click.Path(file_okay=False, dir_okay=True, exists=False), required=True,
              help='The output directory')
@click.option('--steps', type=click.IntRange(0), default=100, show_default=True,
              help='The number of steps to run rofi-tangle with')
@click.option('--samples', type=click.IntRange(1), default=9, show_default=True,
              help='The number of samples for each file')
@click.option('--joblimit', type=click.IntRange(1), default=64, show_default=True,
              help='The maximum number of concurrent jobs')
def gen_tangled_data(world_files: List[str], output_dir: str, steps: int, samples: int, joblimit: int):
    """Generate source data by randomly tangling rofi worlds (in old format) from WORLD_FILES by given number of steps.
    """
    assert shutil.which('rofi-tangle') is not None, \
        'Could not find rofi-tangle'
    os.makedirs(output_dir, exist_ok=True)

    with ThreadPoolExecutor(max_workers=joblimit) as executor:
        jobs: List[futures.Future[None]] = []
        counter = 1
        for world in world_files:
            basename, ext = os.path.splitext(os.path.basename(world))
            for i in range(samples):
                output_filename = f'{basename}-tangled-{i+1}-{steps}{ext}'
                jobs.append(executor.submit(run_rofi_tangle,
                                            world,
                                            os.path.join(output_dir,
                                                         output_filename),
                                            steps,
                                            log=f'Running {counter}/{len(world_files)*samples}'))
                counter += 1

        fs = futures.wait(jobs, return_when='FIRST_EXCEPTION')
        for f in fs.not_done:
            f.cancel()
        for f in fs.done:
            f.result()


if __name__ == '__main__':
    gen_tangled_data()
