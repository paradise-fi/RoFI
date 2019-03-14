# Dependencies

The following libraries need to be installed in order to run the program.

## Armadillo library

Math library used for matrix and vector representation.

### How to install

#### Apt-based systems

You can install the library from the package `libarmadillo-dev`

#### From sources

Follow [the installation notes](http://arma.sourceforge.net/download.html) on the library webpage.

## VTK library

Graphic library used for visualization.

### How to install

#### Apt-based systems

You can install the library from the package `libvtk6-dev`

#### From sources

Follow [the installation notes](https://www.vtk.org/Wiki/VTK/Configure_and_Build) on the library webpage.

What worked for me:

```
git clone git://vtk.org/VTK.git
mkdir VTK-build
cd VTK-build
cmake ../VTK
make -j4
```

In case you have sudo rights on your computer, VTK should be installed and should work with the provided CMakeLists.txt. Otherwise
you have to run cmake with an option `-DVTK_DIR=/path/to/VTK-build `.

# How to run the program

```
mkdir build
cd build
cmake ..
# cmake -DVTK_DIR=/path/to/VTK-build .. # If VTK was installed from sources
make
```

The make should create three executables: `rofi-test`, `rofi-vis` and `rofi-reconfig`.

## Visualizer

```
./rofi-vis ../data/test.in
```

Opens a new window with a visualization of the configuration.

## Reconfiguration

The tool allows to choose from different algorithms for the path computation.

You can choose the algorithm using `--alg` option with two possible arguments:
 
* `bfs`: BFS algorithm with given step size.
* `astar`: A* algorithm with given step size and evaluation function. 

More algorithms will be implemented in the future.

If the A* algorithm is chosen, you can choose from different evaluation functions:

* `trivial`: all configurations have value 1 (default).
* `dCenter`: center difference. Sum of differences of actual centers from goal centers.
* `dJoint`: joint difference. Sum of differences of actual joints from goal joints. 

You can choose the step size (default 90), which defines the size of rotation of 
any joint in one reconfiguration step. Note that descreasing the step size will significantly 
increase the run time of the algorithm.

```
./rofi-reconfig --help
RoFI Reconfiguration: Tool for computing a reconfiguration path.
Usage:
  rofi-reconfig [OPTION...]

  -h, --help      Print help
  -i, --init arg  Initial configuration file
  -g, --goal arg  Goal configuration file
  -s, --step arg  Rotation angle step size in range <0,90>
  -a, --alg arg   Algorithm for reconfiguration: bfs, astar
  -e, --eval arg  Evaluation function for A* algorithm: dCenter, dJoint,
                  trivial
```

Examples:

```
./rofi-reconfig -i ../data/init.in -g ../data/goal.in
```

```
./rofi-reconfig -i ../data/init.in -g ../data/goal.in --alg astar --eval dCenter 
```

Writes a sequence of configurations starting with the initial and ending with the goal configuration.

## How to visualize the result of reconfiguration

```
./rofi-reconfig -i ../data/init.in -g ../data/goal.in > ../data/res.out
./rofi-vis ../data/res.out -m
```

Writes a sequence of configurations to a separate file, then draws many `-m` configurations from one file. The flag must be placed after the file path!