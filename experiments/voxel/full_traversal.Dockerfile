FROM ghcr.io/paradise-fi/rofi.debian:latest

SHELL ["/bin/bash", "-c"]

RUN apt-get install -y --no-install-recommends time jq moreutils

RUN cd /; \
    git clone https://github.com/paradise-fi/RoFI.git --depth 1 --branch master; \
    cd RoFI; \
    source ./setup.sh Release; \
    rcfg desktop; \
    rmake rofi-voxel_full_traversal_bfs

# Form an entrypoint that setups environment
RUN echo $'#!/usr/bin/env bash\n\
    source ./setup.sh -s Release;\n\
    $@\n' > /bin/rofiInvoke; \
    chmod +x /bin/rofiInvoke

WORKDIR /RoFI

ENTRYPOINT ["rofiInvoke"]

# Name: Voxel - full traversal bfs
# Memory limit: 32768 MB
# Tasks:
# [
#     "rofi-voxel_full_traversal_bfs data/configurations/voxel/snake/m1_snake.json --log-counters /artefact/results.json -vv",
#     "rofi-voxel_full_traversal_bfs data/configurations/voxel/snake/m2_snake.json --log-counters /artefact/results.json -vv",
#     "rofi-voxel_full_traversal_bfs data/configurations/voxel/snake/m3_snake.json --log-counters /artefact/results.json -vv",
#     "rofi-voxel_full_traversal_bfs data/configurations/voxel/snake/m4_snake.json --log-counters /artefact/results.json -vv --max 10",
#     "rofi-voxel_full_traversal_bfs data/configurations/voxel/snake/m5_snake.json --log-counters /artefact/results.json -vv --max 8",
#     "rofi-voxel_full_traversal_bfs data/configurations/voxel/snake/m6_snake.json --log-counters /artefact/results.json -vv --max 7",
#     "rofi-voxel_full_traversal_bfs data/configurations/voxel/snake/m7_snake.json --log-counters /artefact/results.json -vv --max 7",
#     "rofi-voxel_full_traversal_bfs data/configurations/voxel/snake/m8_snake.json --log-counters /artefact/results.json -vv --max 6",
#     "rofi-voxel_full_traversal_bfs data/configurations/voxel/snake/m9_snake.json --log-counters /artefact/results.json -vv --max 6",
#     "rofi-voxel_full_traversal_bfs data/configurations/voxel/snake/m10_snake.json --log-counters /artefact/results.json -vv --max 6",
#     "rofi-voxel_full_traversal_bfs data/configurations/voxel/snake/m11_snake.json --log-counters /artefact/results.json -vv --max 6",
#     "rofi-voxel_full_traversal_bfs data/configurations/voxel/snake/m12_snake.json --log-counters /artefact/results.json -vv --max 5",
#     "rofi-voxel_full_traversal_bfs data/configurations/voxel/snake/m13_snake.json --log-counters /artefact/results.json -vv --max 5",
#     "rofi-voxel_full_traversal_bfs data/configurations/voxel/snake/m14_snake.json --log-counters /artefact/results.json -vv --max 5",
#     "rofi-voxel_full_traversal_bfs data/configurations/voxel/snake/m15_snake.json --log-counters /artefact/results.json -vv --max 5"
# ]
