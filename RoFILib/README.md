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

The tool can visualize configurations, create animations and save pictures.

The input file contains representation of one or more configurations separated by an empty line.

You can choose whether to visualize model(s) of configuration(s) in 3D on the screen 
or save picture(s) of configuration(s) using `-s` or `--save` option. You can also specify a path where to save the pictures using `-p` or `--path` option.

If there are more configurations in the input file, you can choose `-a` or `--animation` option which will generete smoother changes between the configurations. If you do not want to animate it, you have to use `-m` or `--many` to specify that there are more than one configuration in the file.

If the animation is chosen, you can specify:

* angle velocity of modules using one of the `-o`, `--omega`, `-f` or `--phi` options
* reconnection time / number of pictures using one of the `-r`, `--recTime`, `-e` or `--recPics` options 

You can also specify settings for camera in a separate file. 
The file can contain following lines:

* `CP x y z`: camera position or
* `CPM xs xe ys ye zs ze`: camera position move xstart -> xend, ystart -> yend, zstart -> zend
* `CF x y z`: camera focal point or
* `CFM xs xe ys ye zs ze`: camera focal point move xstart -> xend, ystart -> yend, zstart -> zend
* `CV x y z`: camera view up or
* `CVM xs xe ys ye zs ze`: camera view up move xstart -> xend, ystart -> yend, zstart -> zend

Default values use mass center of the first configuration. 

```
RoFI Visualizer: Tool for visualization of configurations and creating animations.
Usage:
  rofi-vis [OPTION...]

  -h, --help         Print help
  -s, --save         Save picture to file
  -a, --animation    Create animation from configurations
  -c, --camera arg   Camera settings file
  -p, --path arg     Path where to save pictures
  -o, --omega arg    Maximal angular velocity in 1°/s
  -f, --phi arg      Maximal angle diff in ° per picture
  -r, --recTime arg  Time in seconds for reconnection
  -e, --recPics arg  Number of pictures for reconnection
  -i, --input arg    Input config file
  -m, --many         Many configurations in one file
```

Examples:

```
./rofi-vis -i ../data/test.in
```

Opens a new window with a visualization of the configuration.

```
./rofi-vis -i ../data/res.out -s --path ../data/res -a 

```

Generates some intersteps (animation) and saves pictures to the ../data/res directory.

```
./rofi-vis -i ../data/res.out -s --path ../data/res -a -c ../data/1.cam

```

Same as above with specified camera settings.

```
./rofi-vis -i ../data/res.out -s --path ../data/res -a -c ../data/1.cam --omega 15 --recTime 2

```


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
./rofi-vis -i ../data/res.out -m
```

Writes a sequence of configurations to a separate file, then draws many `-m` configurations from one file. 
