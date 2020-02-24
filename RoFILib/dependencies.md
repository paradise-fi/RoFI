# Dependencies

The following libraries need to be installed in order to compile and use RoFILib.

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

## FFmpeg

Tool for creating videos from single pictures

### How to install

Follow [the installation notes](https://ffmpeg.org/download.html) on the framework webpage.

## Inkscape

Vector graphics software used for exporting bitmap pictures from vector graphics.

### How to install

Follow [the installation notes](https://inkscape.org/release/) on the webpage.

## OpenMPI library

Library for distributed computing.

### How to install

#### Apt-based systems

You can install the library from the package `libopenmpi-dev` and `openmpi-bin`

#### From sources

Follow [the installation notes](https://www.open-mpi.org/software/ompi/) on the library webpage.
