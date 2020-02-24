# Reconfig

The tool allows to choose from different algorithms for the path computation.

You can choose the algorithm using `--alg` option with two possible arguments:

* `bfs`: BFS algorithm with given step size and parallel bound.
* `astar`: A* algorithm with given step size, parallel bound and evaluation function.
* `rrt`: RRT algorithm with given step size.

More algorithms will be implemented in the future.

If the A* algorithm is chosen, you can choose from different evaluation functions:

* `trivial`: all configurations have value 1 (default).
* `dMatrix`: module matrices difference. Sum of differences of actual matrices from goal matrices.
* `dCenter`: module center difference. Sum of differences of actual centers from goal centers.
* `dJoint`: joint difference. Sum of differences of actual joints from goal joints.
* `dAction`: action difference. Count of rotation actions and reconnection actions needed from
actual configuration to goal configuration.

You can choose the step size (default 90), which defines the size of rotation of
any joint in one reconfiguration step. Note that decreasing the step size will significantly
increase the run time of the algorithm. Found solutions should be potentially longer.

You can choose how many actions can occur in parallel, which will allow more rotations
and reconnections in one step. Note that increasing the paralle bound will significantly
increase the runtime of the algorithm. Found solutions should be potentially shorter.

```
./rofi-reconfig --help
RoFI Reconfiguration: Tool for computing a reconfiguration path.
Usage:
  rofi-reconfig [OPTION...]

  -h, --help          Print help
  -i, --init arg      Initial configuration file
  -g, --goal arg      Goal configuration file
  -s, --step arg      Rotation angle step size in range <0,90>
  -a, --alg arg       Algorithm for reconfiguration: bfs, astar, rrt
  -e, --eval arg      Evaluation function for A* algorithm: dMatrix, dCenter,
                      dJoint, dAction, trivial
  -p, --parallel arg  How many parallel actions are allowed: <1,...>
```

Examples:

```
./rofi-reconfig -i ../data/init.in -g ../data/goal.in
```

```
./rofi-reconfig -i ../data/init.in -g ../data/goal.in --alg astar --eval dCenter
```

Writes a sequence of configurations starting with the initial and ending with the goal configuration.