# RoFI simulator

## Prerequisites

To run & develop RoFI simulator you have to install Gazebo & its development
libraries. On APT-based systems you can run:

```
apt install apt install gazebo9 libgazebo9-dev
```

## Compilation

Just run `make` in the simulation directory. It will create a build directory
and invoke CMake. The compiled binaries and Gazebo plugins are in the `build`
directory.

## How To Use It

To use RoFI models and plugins correct Gazebo paths have to be set up. To ease
this step, you can run `runGazebo.sh` which will set them and runs gazebo.

In Gazebo you can add models and control the simulation. Read more about the
Gazebo GUI in [the official
documentation](http://gazebosim.org/tutorials?tut=guided_b2&cat=).

To control the join speed (demo functionality), run:

```
build/tools/publisherDemo <desiredJointSpeed>
```

## Development Guide

As the simulator is based on Gazebo, follow the [Gazebo
tutorials](http://gazebosim.org/tutorials). Especially, [the control plugin
tutorial](http://gazebosim.org/tutorials?tut=guided_i5) is a good starting point
for our purposes.

Another usefull tutorials:
- [Custom messages](http://gazebosim.org/tutorials?tut=custom_messages&cat=transport)
- [SDF format specification](http://sdformat.org/spec)

