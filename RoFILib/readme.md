# RoFILib

RoFILib is a collection of libraries and related tool for the RoFI platform.

It contains tools related to:

- `configuration` provides means to represent and modify a configuration of a
  RoFIBot. `util` contains small tools to automate common tasks.
- reconfiguration
  - `reconfig` can compute reconfiguration between two arbitrary configurations
    via state-space search
  - `smtreconfig` can compute reconfiguration between two arbitrary configurations
    via reduction to SMT
  - `distribute` an implementation of distributed algorithm for
    reconfiguration
- visualization of reconfiguration (see `visualizer`)

## Compiling

The whole library is compiled by standard CMake. The library and tools have the following dependencies specified in [dependiencies.md](dependencies.md).

