import json
from typing import Tuple
import argparse
import sys
import numpy as np

import matplotlib.pyplot as plt
import matplotlib.patheffects as pe
from math import inf


def sort_edge(a: int, b: int) -> Tuple[int, int]:
    if a < b:
        return a, b
    else:
        return b, a


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='Display Joints in 3D space with gravitation vector')

    parser.add_argument('-g', '--gravitation', type=str, default='[0,0,10.0]',
                        help='The gravitation vector')
    parser.add_argument('-o', '--output_file', type=str, help='The output file',
                        default='output.png')
    parser.add_argument('-a', '--azimuth', type=int, help='Rotation of view',
                        default=45)
    parser.add_argument('-e', '--elevation', type=int, help='Elevation of view',
                        default=22)
    args = parser.parse_args()
    gravitation = np.array(json.loads(args.gravitation))

    print(gravitation)

    x_min = inf
    y_min = inf
    z_min = inf

    x_max = -inf
    y_max = -inf
    z_max = -inf

    inputConfig = sys.stdin.read()
    config = json.loads(inputConfig)

    fig = plt.figure(figsize=(25, 25))
    # ax = fig.gca(projection='3d')
    ax = fig.add_subplot(111, projection='3d')

    edges_visited: set[Tuple[int, int]] = set()

    for joint_id, joint in config["joints"].items():
        coors = joint["coors"]

        if coors[0] < x_min:
            x_min = coors[0]
        if coors[1] < y_min:
            y_min = coors[1]
        if coors[2] < z_min:
            z_min = coors[2]

        if coors[0] > x_max:
            x_max = coors[0]
        if coors[1] > y_max:
            y_max = coors[1]
        if coors[2] > z_max:
            z_max = coors[2]

        j_id = joint["id"]
        color = 'r' if joint["is_bounded"] else \
            'g' if joint["is_wall"] else 'b'

        ax.scatter(coors[0], coors[1], coors[2], color=color, s=20*4**2)
        ax.text(coors[0], coors[1], coors[2], joint_id,
                size=20, zorder=999, color='k',
                path_effects=[pe.withStroke(linewidth=4, foreground="white")])

        for neighbor_id in joint["neighbors"]:
            edge = sort_edge(j_id, neighbor_id)

            if edge not in edges_visited:
                neighbor = config["joints"][str(neighbor_id)]
                n_coors = neighbor["coors"]

                x_values = [coors[0], n_coors[0]]
                y_values = [coors[1], n_coors[1]]
                z_values = [coors[2], n_coors[2]]

                ax.plot(x_values, y_values, z_values, 'bo', linestyle="--")
                edges_visited.add(edge)

    ax.set_xlabel('x')
    ax.set_ylabel('y')
    ax.set_zlabel('z')

    gravitation_start_point = [
        x_min if gravitation[0] >= 0 else x_max,
        y_min if gravitation[1] >= 0 else y_max,
        z_min if gravitation[2] >= 0 else z_max,
    ]
    grav_length = min(x_max - x_min, y_max - y_min, z_max - z_min)
    gravitation = gravitation / np.linalg.norm(gravitation) * grav_length

    ax.quiver(
        gravitation_start_point[0],
        gravitation_start_point[1],
        gravitation_start_point[2],
        gravitation[0],
        gravitation[1],
        gravitation[2],
        color="green"
    )

    ax.set_aspect('equal', adjustable='box')

    ax.axes.set_xlim3d(left=x_min, right=x_max)
    ax.axes.set_ylim3d(bottom=y_min, top=y_max)
    ax.axes.set_zlim3d(bottom=z_min, top=z_max)

    ax.view_init(elev=args.elevation, azim=args.azimuth)
    print(args.output_file)
    plt.savefig(args.output_file, bbox_inches='tight')
