RoFI Compilation
================

We provide an overview of the build process of the project. One important note,
the RoFI build process works on Linux nad MacOS. You cannot build the project on
Windows at the moment.

To compile the project, ensure that you have Python 3, CMake and make installed.
The compilation suites might require additional dependencies; see the suites
pages in the menu on the left for more details.

If you are interested only in the "step-by-step tutorial" on building, feel free
to skip to the section :ref:`quick-start-example`.

.. _environment:

Build environment
-----------------

To lift the burden of compiling and running targets we provide a script to setup
proper development environment. This setups ``PATH`` to management tools,
compiled binaries and also provides other environmental variables required by
the build.

When you set up the environment, you specify which type of environment you want:
either ``Release`` (the default) or ``Debug``.

So whenever you want to work with the project, invoke in your shell:

.. code-block:: sh

    $ source setup.sh Release

This will setup your current shell for RoFI. If you want to change the
configuration, simply invoke ``source setup.sh Debug`` to e.g., switch to a
debug configuration. Or invoke ``teardown`` to return your shell to original
state.

If you would like to also compile the firmware for the physical robots, you
should invoke the setup command with ``-i``. This will setup ESP-IDF, which is
necessary to compile suite ``esp32``.

Once you setup the environment, you have management commands available in your
shell (e.g., ``rcfg`` and ``rmake``). Then you can proceed to configuration of
compilation suites and building targets.

Note that if you would like to make ``setup.sh`` silent (e.g., when benchmarking
or running in CI), you can pass the option ``-s`` to make it silent.

Also, if you would like to be able to detect if you are in RoFI-aware shell, you
can add the environmental variable ``ROFI_SHELL_INDICATOR`` to your prompt. It
will show and icon of the robot and, also, the marking of currently active
configuration.

The build directories are constructed as
``build.${ROFI_BUILD_PREFIX}${SELECTED_COFIGURATION}``. Therefore, if you have a
need for multiple build directories, you can specify environmental variable
``ROFI_BUILD_PREFIX`` before invoking ``setup.sh`` to distinguish them.


Compilation suites
------------------

The project compilation is organized into so-called *suites*. Suite is a
collection of entities (not necessarily programs), that are build in a similar
fashion (e.g., using a standard desktop compiler, compiler for microcontrollers,
documentation building tool). Note that the suites are organized mainly
according to build process, they might not be grouped logically. A suite is the
smallest unit, you can *configure for compilation*.

Currently, there are the following suites:

-  **desktop**: Programs and libraries running on a desktop computer. That
   includes simulators, benchmarking tools etc.

-  **hardware**: Hardware components for RoFI. Building this
   target creates e.g., `.stl` files for printing, ZIP packages containing
   gerber files for PCB production, bill-of-materials, etc.

-  **stm32**: Firmware for the microcontrollers on hardware modules based on
   STM32 microcontrollers. This does not include the RoFI programs. Think of
   this as the part of the hardware, that just have to be build separately.

-  **esp32**: The actual programs that are deployed to the RoFI modules.

-  **doc**: This documentation.

The suites are represented by directories in ``${PROJECT_ROOT}/suites``.
Basically, each suite is a separate CMake project defining the build process for
the given suite. If a suite has development dependencies, they are specified in
the ``readme.md`` file in the suite directory.

You can setup the compilation units via invoking:

.. code-block:: sh

    $ rcfg desktop firmware                 # Setup desktop and firmware suites
    $ rcfg desktop -- -DBUILD_HEADLESS=true # Setup desktop with headless build option
    $ rcfg -desktop                         # Unconfigure the desktop suite

Note that tab-completion works for this command. Once you setup the suites, you
can proceed to the compilation. You can also pass extra options to the
underlying CMake project via ``--`` -- everything after ``--`` is passed to the
CMake.

We recommend you to configure only the suites your are interested in so you can
minimize the number of dependencies you have to met (e.g., it makes no sense to
configure the firmware, thus install the embedded compiles, if you only plan to
develop desktop applications).

You can learn about the suite-specific configurations in the following sections:

.. toctree::
    suites/desktop
    suites/doc


Compilation targets
-------------------

Each suite provides usually several compilation targets (just like CMake or make
does). Targets are the smallest unit you can build. Once you have configured
suites, you can list all possible targets via:

.. code-block:: sh

    $ rmake --list

With output similar to the following:

.. code-block:: none

    Available suites: firmware, desktop, rofiFirmware, hardware, doc

    Available targets for suite firmware:
        None

    Available targets for suite desktop:
        atoms-test: softwareComponents/atoms
        configuration: softwareComponents/configuration
        generators: softwareComponents/configuration
        configuration-test: softwareComponents/configuration
        rofi-reconfig: softwareComponents/reconfig
        reconfig: softwareComponents/reconfig
        reconfig-test: softwareComponents/reconfig
        smt-reconfig-lib: softwareComponents/smtreconfig
        smt-reconfig: softwareComponents/smtreconfig
        smt-reconfig-test: softwareComponents/smtreconfig
        rofi-vis: tools/visualizer
        rofi-vis-test: tools/visualizer
        rofi-app: tools/visualizer
        rofi-generate: tools/generate
        rofi-emptyDockFill: tools/emptyDockFill
        rofi-topology2dot: tools/topology2dot

    Available targets for suite rofiFirmware:
        None

    Available targets for suite hardware:
        None

    Available targets for suite doc:
        doc: doc

The output gives you list of all suites, targets and their location in the
source tree. Note that only configured targets are shown. To actually compile a
target, you can specify one more targets to ``rmake``:

.. code-block:: sh

    $ rmake desktop    # Compile all targets from the suite desktop
    $ rmake rofi-vis   # Compile only target rofi-vis (and its dependencies)
    $ rmake tools/     # Compile all targets that are in the source tree under prefix
    $ rmake tools/ doc # You can mix various types of targets
    $ rmake --all      # Compile everything

Note that once you compile a target, you can directly run the binaries as they
are in ``PATH``. If you would like to inspect the build artifacts, they are
located in the directory ``build.{Configuration}/{suiteName}``.

``rmake`` also recognizes phony targets ``clean`` and ``clean-<suitename>`` that
clean all suites or given suite respectively.

In case you would like to run clang-tidy on the sources, you can use ``rmake
--tidy <pattern>``. The clang-tidy will be invoked on all sources matching the
pattern.

The ``rmake`` tool internally uses Ninja (with fallback to Make). Since Ninja is
much faster (especially on incremental builds) we advise you to have it
installed.

.. _flashing:

Flashing robots and hardware modules
------------------------------------

The RoFI environment offers two tools: ``rflash`` and ``rmonitor`` that allows
you to flash images to hardware devices and to monitor their debug outputs. The
tools are automatically deduced from the image.

Both tools have the same usage:

.. code-block:: sh

    $ rflash <image_name> -- <optional extra arguments for the underlying tool>
    $ rmonitor <image_name> -- <optional extra arguments for the underlying tool>

Note that the image is mandatory for the monitor command as it is used to decode
stack traces and also, to determine the correct connection parameters.

.. _advanced_config:

Advanced configuration of the build process
-------------------------------------------

In the case when you need to override the default tools and path used, e.g.,
when you want to force a particular version of compiler, there are two files
that are sourced into the environment during invocation of ``source setup.sh``:
``~/.rofi.pre.env`` and ``~/.rofi.post.env``. If these files exist, they are
sourced into the environment. Here you can, e.g., enforce a particular compiler
version:

.. code-block:: sh

    # .rofi.post.env

    export CC=$(command -v gcc-11)
    export CXX=$(command -v g++-11)


The extra tools that ``setup.sh`` downloads, are placed in a directory marked by
the environmental variable ``ROFI_TOOLS_PATH``. If the variable is not set, it
defaults to ``ROFI_ROOT/build.deps``. You might want to set this variable in the
case when you work on multiple worktrees to save disk space -- ESP-IDF consumes
about 2.5 GB of disk space.

.. _quick-start-example:

Quick start example
-------------------

To quickly start with RoFI development, we provide a short step-by-step guide:

.. code-block:: sh

    $ source setup.sh Debug # Setup your terminal for compiling RoFI in Debug mode
    $ rcfg desktop doc      # Configure the suites you want
    $ rmake --all           # Build everything you have configured
    $ rmake tools/          # ... or build only all tools
    $ rofi-vis              # Run the compiled tool


Quick start Simulator example
------------------------------

To quickly start with simulating RoFI, we provide a short step-by-step guide.

Compiling and running the RoFI module code for simulators is the same,
only the setup for simulator changes.

GazeboSim
~~~~~~~~~

.. code-block:: sh

    $ source setup.sh Debug                     # Setup your terminal in Debug mode
                                                # This also sets the necessary gazebo paths and variables
    $ rcfg desktop && rmake desktop             # Configure the desktop suite and build it
    $ gazebo worlds/two_modules.world --verbose # Run gazebo. We recommend using --verbose for more info

SimpleSim
~~~~~~~~~

.. code-block:: sh

    $ source setup.sh Debug                      # Setup your terminal in Debug mode
    $ rcfg desktop && rmake desktop              # Configure the desktop suite and build it
    $ rofi-simplesim --help                      # Display help
    $ rofi-simplesim data/configurations/init.in # Run simplesim


Module code
~~~~~~~~~~~

.. attention::
    When compiling the module code that you want to run in a simulator, make sure
    to link it with the correct RoFI HAL library ``rofi_hal_sim``.

In a different terminal then where you run the simulator,
run the module code for each module that you want to control:

.. code-block:: sh

    $ source setup.sh    # Setup your terminal
    $ basicJointPosition # Run the compiled module code

If you are interested in controlling the simulation via Python instead of C++,
you can follow example ``examples/simulator/pythonInterface``:

.. code-block:: sh

    $ python3 examples/simulator/pythonInterface/oscilate.py
