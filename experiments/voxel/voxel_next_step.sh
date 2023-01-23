#!/usr/bin/env bash
rofi-convert $1 - --if old --of voxel | rofi-voxel_next_step_count - -vv --output /artefact/results.json
