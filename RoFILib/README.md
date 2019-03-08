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

```
./rofi-reconfig ../data/init.in ../data/goal.in
```

Writes a sequence of configurations starting with the initial and ending with the goal configuration.
