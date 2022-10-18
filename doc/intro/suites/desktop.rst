Desktop Suite
=============

The basic suite containing mainly the tools for defining and manipulating RoFI
shape, algorithmic libraries and benchmarks.

Dependencies
------------

Refer to ``suites/desktop/Dockerfile`` for the up-to-date reference build and
development environment. On top of basic C/C++ dependencies, the following extra
libraries are required to be installed:

-  `Armadillo <http://arma.sourceforge.net/>`__: for APT-based system, install
   ``libarmadillo-dev``
-  `VTK >=7 <http://vtk.org/>`__: for APT-based system, install ``libvtk7-dev``
-  `QT 5 and QT-VTK <http://qt.io/>`__: for APT-based system, install
   ``libvtk7-qt-dev qtdeclarative5-dev``
-  `GazeboSim >=11 <http://gazebosim.org/>`__: for APT-based system, install
   ``libgazebo11-dev``. This dependecy can be removed using the build options.
-  `Z3 <https://github.com/Z3Prover/z3>`__: this dependency is only needed for
   compiling SMT reconfiguration.

On top of the build dependencies, FFMpeg and Inkscape are required to use the
animation feature of the visualizer.

Build options
-------------

-  ``BUILD_HEADLESS`` (default ``FALSE``): Build only parts of the project that
   can run headlessly. I.e., does not compile visualizer nor rofi-app.
-  ``BUILD_GAZEBO`` (default ``TRUE``): Build parts that require gazebo libraries.
-  ``BUILD_RUST`` (default ``TRUE``): Build parts that contain or depend on Rust.
-  ``BUILD_SMTRECONFIG`` (default ``FALSE``): Defines whether to build the
   library for reconfiguration via reduction to SMT. The library is not directly
   applicable and also brings heavy dependency in the form of Z3, thus it makes
   sense to do not compile it by-default.
